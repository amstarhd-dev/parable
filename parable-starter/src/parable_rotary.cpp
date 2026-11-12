// Acoustic architecture narrowly adapted from MIT cwbudde/algo-dsp rotary:
// two-band split, angle directivity and Doppler history. Credits/pin/notices in
// third_party/rotary. Belt/motor equations are a Parable-local reduced model.
#include "parable_rotary.h"
#include "rotary_delay.h"
#include <cmath>
#include <limits>
namespace parable::dsp {
namespace {
constexpr double pi=3.1415926535897932384626433832795,twoPi=2*pi;
double clamp(double x,double lo,double hi) noexcept {return x<lo?lo:x>hi?hi:x;}
double wrap(double x) noexcept {if(x>=twoPi)x-=twoPi;else if(x<0)x+=twoPi;return x;}
void increment(uint32_t& n) noexcept {if(n!=std::numeric_limits<uint32_t>::max())++n;}
}
bool Rotary::prepare(float rate,RotaryConfig config) noexcept {
    if(!std::isfinite(rate)||rate<32000||rate>96000||!std::isfinite(config.mix)||config.mix<0||config.mix>1)return false;
    config_=config;rate_=rate;dt_=1./rate;
    crossover_=float(1-std::exp(-twoPi*800/rate));
    hornDark_=float(1-std::exp(-twoPi*1500/rate));hornBright_=float(1-std::exp(-twoPi*8000/rate));
    for(uint32_t i=0;i<kSineSegments;++i)sine_[i]=float(std::sin(twoPi*i/kSineSegments));
    sine_[kSineSegments]=sine_[0];reset();return true;
}
void Rotary::reset() noexcept {
    horn_={};drum_={};
    horn_.motorOmega=horn_.omega=(.12*twoPi*.8-.02)/(.12+.001);
    drum_.motorOmega=drum_.omega=(.12*twoPi*.6-.02)/(.12+.002);
    horn_.beltPhase=drum_.beltPhase=pi;
    write_=containment_=0;low_=color_=hornWet_=drumWet_=0;delayH_=delayB_=0;
    lowDelay_.fill(0);highDelay_.fill(0);running_=rate_>0;
}
double Rotary::cosine(double angle) const noexcept {
    const double p=wrap(angle+pi/2)*(kSineSegments/twoPi);
    const auto index=static_cast<uint32_t>(p);if(index>=kSineSegments)return sine_[0];
    const double f=p-index,a=sine_[index],difference=sine_[index+1]-a;
    // Cubic Hermite, analytic endpoint slopes from the same quarter-shifted
    // table. Linear lookup's error floor masked integration-rate convergence.
    const double m0=(twoPi/kSineSegments)*sine_[(index+64)&255];
    const double m1=(twoPi/kSineSegments)*sine_[(index+65)&255];
    return clamp(((m0+m1-2*difference)*f+(3*difference-2*m0-m1))*f*f+m0*f+a,-1,1);
}
void Rotary::advance(RotorMotion& s,bool horn,double command) noexcept {
    const double jm=horn?.0002:.001,jr=horn?.002:.015;
    const double b=horn?.001:.002,maximum=horn?.14:.22;
    const double slow=horn?.8:.6,fast=horn?6.5:5.;
    const double target=twoPi*(slow+(fast-slow)*command);
    s.driveNm=clamp(.12*(target-s.motorOmega),-maximum,maximum);
    s.loadNm=b*s.omega+.02*clamp(s.omega/.5,-1,1);
    const double lobe=clamp(cosine(s.beltPhase),0,1),lobe2=lobe*lobe;
    const double wear=horn&&config_.wornHornBelt?.96:0;
    s.staticNm=(horn?.22:.35)*(1-wear*lobe2*lobe2);
    s.requiredNm=jr*(s.driveNm-s.loadNm)/(jm+jr)+s.loadNm;
    const double oldMotor=s.motorOmega,oldRotor=s.omega,relative=oldMotor-oldRotor;
    s.released=s.captured=false;s.captureLossJ=0;
    if(!s.slipping&&std::abs(s.requiredNm)>s.staticNm) {s.slipping=true;s.released=true;increment(s.releases);}
    if(s.slipping) {
        const double direction=std::abs(relative)>1e-12?relative:s.requiredNm;
        s.beltNm=.7*s.staticNm*(direction<0?-1:1);
        s.motorOmega+=dt_*(s.driveNm-s.beltNm)/jm;
        s.omega+=dt_*(s.beltNm-s.loadNm)/jr;
        if(relative*(s.motorOmega-s.omega)<=0&&std::abs(s.requiredNm)<=s.staticNm) {
            const double common=(jm*s.motorOmega+jr*s.omega)/(jm+jr);
            s.captureLossJ=.5*jm*s.motorOmega*s.motorOmega+.5*jr*s.omega*s.omega-.5*(jm+jr)*common*common;
            s.motorOmega=s.omega=common;s.slipping=false;s.captured=true;increment(s.captures);
        }
    } else {
        s.beltNm=s.requiredNm;s.motorOmega=s.omega=oldRotor+dt_*(s.driveNm-s.loadNm)/(jm+jr);
    }
    // Emergency numerical containment is counted, never credited as a fault.
    if(std::abs(s.motorOmega)>80||std::abs(s.omega)>80||!std::isfinite(s.motorOmega)||!std::isfinite(s.omega)) {
        s.motorOmega=clamp(std::isfinite(s.motorOmega)?s.motorOmega:0,-80,80);
        s.omega=clamp(std::isfinite(s.omega)?s.omega:0,-80,80);increment(containment_);
    }
    s.motorAngle=wrap(s.motorAngle+dt_*(oldMotor+s.motorOmega)*.5);
    s.angle=wrap(s.angle+dt_*(oldRotor+s.omega)*.5);
    s.beltPhase=wrap(s.beltPhase+dt_*(oldMotor+s.motorOmega)*.125);
}
float Rotary::process(float input,float speedCommand) noexcept {
    if(!running_)return 0;
    const float x=std::isfinite(input)?float(clamp(input,-1,1)):0;
    const double command=std::isfinite(speedCommand)?clamp(speedCommand,0,1):0;
    advance(horn_,true,command);advance(drum_,false,command);
    const double h=cosine(horn_.angle),b=cosine(drum_.angle);
    low_+=crossover_*(x-low_);
    const float coefficient=float(hornDark_+(hornBright_-hornDark_)*(.5+.5*h));
    color_+=coefficient*(x-low_-color_);
    lowDelay_[write_]=low_;highDelay_[write_]=color_;
    delayH_=rate_*(1-.18*h)/343;delayB_=rate_*(1-.091*b)/343;
    const double gainH=clamp((.65+.85*h+.20*(2*h*h-1))/1.7,.05/1.7,1);
    const double gainB=clamp((.65+.35*b+.08*(2*b*b-1))/1.08,.05/1.08,1);
    hornWet_=float(gainH*rotary_detail::read(highDelay_.data(),write_,delayH_)/3);
    drumWet_=float(gainB*rotary_detail::read(lowDelay_.data(),write_,delayB_)/3);
    const double wet=double(hornWet_)+drumWet_;
    if(++write_==kDelayCells)write_=0;
    return float((1-config_.mix)*x+config_.mix*wet);
}
} // namespace parable::dsp
