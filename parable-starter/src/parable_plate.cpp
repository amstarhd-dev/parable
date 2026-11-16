#include "parable_plate.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <utility>
namespace parable::dsp {
namespace {
constexpr float pi = 3.14159265358979323846f;
// Dattorro's nominal 29761 Hz design (original J. Dattorro, JAES 45(9), 1997).
constexpr std::array<float,12> lengths = {142,107,379,277,672,908,4453,4217,1800,2656,3720,3163};
inline float clamp1(float x) noexcept {return std::clamp(x,-1.f,1.f);}
inline void satInc(std::uint32_t &n) noexcept {if (n<std::numeric_limits<std::uint32_t>::max()) ++n;}
}
float Plate::Delay::tap(float samples) const noexcept {
    if(data.empty())return 0;
    // pos is the next write position; tap(1) reads the latest written sample.
    const float d=std::clamp(samples,1.f,float(data.size()-2));
    const unsigned whole=static_cast<unsigned>(d);
    const float frac=d-whole;
    const unsigned n=static_cast<unsigned>(data.size());
    const unsigned i=(pos+n-whole)%n, j=(i+n-1)%n;
    return data[i] + frac*(data[j]-data[i]);
}
void Plate::Delay::push(float v) noexcept {data[pos]=v;if(++pos==data.size())pos=0;}
void Plate::Delay::reset() noexcept {std::fill(data.begin(),data.end(),0.f);pos=0;}
float Plate::apf(unsigned i,float x,float g,float d) noexcept {
    auto &line=lines_[i+1];const float previous=line.tap(d);
    const float w=x+g*previous;
    line.push(w);return previous-g*w;
}
bool Plate::prepare(float sampleRate,PlateControls c) noexcept {
    if(!std::isfinite(sampleRate)||sampleRate<44100.f||sampleRate>96000.f||
       !std::isfinite(c.decay)||c.decay<0.f||c.decay>0.95f||
       !std::isfinite(c.dampingHz)||c.dampingHz<500.f||c.dampingHz>18000.f||
       !std::isfinite(c.wet)||c.wet<0.f||c.wet>1.f)return false;
    try {
        Plate next;next.sampleRate_=sampleRate;next.controls_=c;
        const float ratio=sampleRate/29761.f;
        next.predelay_=sampleRate*0.006f;
        next.lines_[0].data.assign(static_cast<std::size_t>(std::ceil(next.predelay_))+4,0.f);
        for(unsigned i=0;i<12;++i){
            next.lengths_[i]=lengths[i]*ratio;
            // 8 samples modulation margin for the two tank diffusers.
            const float extra=(i==4||i==5)?12.f:0.f;
            next.lines_[i+1].data.assign(static_cast<std::size_t>(std::ceil(next.lengths_[i]+extra))+4,0.f);
        }
        for(unsigned i=0;i<257;++i)next.sine_[i]=std::sin(2.f*pi*i/256.f);
        next.modFreqA_=256.f*0.9f/sampleRate;next.modFreqB_=256.f*0.73f/sampleRate;
        next.inCoeff_=1.f-std::exp(-2.f*pi*13000.f/sampleRate);
        next.dampCoeff_=1.f-std::exp(-2.f*pi*c.dampingHz/sampleRate);
        next.running_=true;
        *this=std::move(next);return true;
    } catch(const std::bad_alloc&) {return false;}
}
void Plate::reset() noexcept {
    for(auto &line:lines_) line.reset();
    inputLP_=dampLP_L_=dampLP_R_=0.f;
    oscX_=oscV_=collisionReturn_=sinePhaseA_=sinePhaseB_=0.f;contactSide_=0;
    probe_={};invalid_=clipped_=nonfinite_=0;running_=sampleRate_>0;
}
std::size_t Plate::bufferBytes() const noexcept {
    std::size_t bytes=0;for(const auto &d:lines_)bytes+=d.data.capacity()*sizeof(float);return bytes;
}
float Plate::process(float input) noexcept {
    if(!running_)return 0.f;
    if(!std::isfinite(input)){satInc(invalid_);return 0.f;}
    if(std::abs(input)>1.f){satInc(clipped_);input=clamp1(input);}
    // A collision returns an impulse into the next sample's *input diffusion*;
    // it is not a separately mixed buzz track. Bounded energy injection.
    const float injected=std::clamp(input+collisionReturn_,-1.f,1.f);collisionReturn_=0;
    float x=lines_[0].tap(predelay_);lines_[0].push(injected);
    inputLP_+=inCoeff_*(x-inputLP_);
    x=apf(0,inputLP_,.75f,lengths_[0]);
    x=apf(1,x,.75f,lengths_[1]);
    x=apf(2,x,.625f,lengths_[2]);
    x=apf(3,x,.625f,lengths_[3]);
    const float d=controls_.decay;
    float p=x+d*decayTap(10,lengths_[10]);
    float q=x+d*decayTap(11,lengths_[11]);
    // Small, deterministic tank modulation to reduce stationary metallic combs.
    sinePhaseA_+=modFreqA_;sinePhaseB_+=modFreqB_;
    if(sinePhaseA_>=256.f)sinePhaseA_-=256.f;
    if(sinePhaseB_>=256.f)sinePhaseB_-=256.f;
    const unsigned ai=static_cast<unsigned>(sinePhaseA_),bi=static_cast<unsigned>(sinePhaseB_);
    const float sa=sine_[ai]+(sine_[ai+1]-sine_[ai])*(sinePhaseA_-ai);
    const float sb=sine_[bi]+(sine_[bi+1]-sine_[bi])*(sinePhaseB_-bi);
    p=apf(4,p,-.7f,lengths_[4]+2.f*sa);
    q=apf(5,q,-.7f,lengths_[5]+2.f*sb);
    // Left tank: APF672, delay4453+damper, APF1800, delay3720.
    float dl=decayTap(6,lengths_[6]);lines_[7].push(p);
    dampLP_L_+=dampCoeff_*(dl-dampLP_L_);
    p=apf(8,d*dampLP_L_,.5f,lengths_[8]);lines_[11].push(p);
    // Right tank: APF908, delay4217+damper, APF2656, delay3163.
    float dr=decayTap(7,lengths_[7]);lines_[8].push(q);
    dampLP_R_+=dampCoeff_*(dr-dampLP_R_);
    q=apf(9,d*dampLP_R_,.5f,lengths_[9]);lines_[12].push(q);
    // Dattorro's fourteen output taps, nominal positions / 29761 Hz.
    const float r=sampleRate_/29761.f;
    const float L=0.6f*(
        decayTap(7,266*r)+decayTap(7,2974*r)-decayTap(9,1913*r)+
        decayTap(11,1996*r)-decayTap(6,1990*r)-decayTap(8,187*r)-decayTap(10,1066*r));
    const float R=0.6f*(
        decayTap(6,353*r)+decayTap(6,3627*r)-decayTap(8,1228*r)+
        decayTap(10,2673*r)-decayTap(7,2111*r)-decayTap(9,335*r)-decayTap(11,121*r));
    const float body=.5f*(L+R);
    // Reduced mechanical mount: a damped transverse local mode driven by the
    // diffuse plate-like field. Nominal displacement units, NOT measured mm.
    // Changing clearance alone introduces recurring unilateral contact.
    const float omega=2.f*pi*680.f,dt=1.f/sampleRate_,zeta=.09f;
    oscV_+=dt*(omega*omega*(2.5f*body-oscX_)-2.f*zeta*omega*oscV_);
    oscX_+=dt*oscV_;
    const float gap=controls_.damagedMount?0.045f:3.0f;
    float impulse=0.f;
    if(std::abs(oscX_)>gap){
        const int direction=oscX_>0?1:-1;
        oscX_=direction*gap;
        if(direction*oscV_>0){
            if(contactSide_!=direction){
                const float before=oscV_;
                oscV_=-0.19f*before; // restitution <1, dissipative impact
                impulse=oscV_-before;
                if(probe_.contacts<std::numeric_limits<std::uint64_t>::max())++probe_.contacts;
                collisionReturn_=std::clamp(0.85f*impulse/omega,-.12f,.12f);
            }else{
                oscV_=0; // pressed against constraint: no repeated collision event
            }
            contactSide_=direction;
        }
    }else if(contactSide_!=0 && std::abs(oscX_)<gap*0.985f){
        contactSide_=0; // actual separation permits the next impact
    }
    probe_.contactImpulse=impulse;
    probe_.suspensionDisplacement=oscX_;
    probe_.suspensionVelocity=oscV_;
    probe_.peakDisplacement=std::max(probe_.peakDisplacement,std::abs(oscX_));
    probe_.wetLeft=L;probe_.wetRight=R;
    const float out=(1.f-controls_.wet)*input+controls_.wet*0.7f*body;
    if(!std::isfinite(out)){satInc(nonfinite_);return 0.f;}
    return std::clamp(out,-1.f,1.f);
}
} // namespace parable::dsp
