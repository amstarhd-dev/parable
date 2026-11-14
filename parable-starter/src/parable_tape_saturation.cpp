#include "parable_tape_saturation.h"
#include "tape_saturation_material.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace parable::dsp {
namespace {
namespace model=tape_saturation_detail;
constexpr double pi=model::pi,speed=.381,recordGap=7e-6,depth=8.3e-6;
constexpr double halfspan=40e-6,transit=2*halfspan/speed,biasHz=100000;
constexpr double audioDeep=.6*300*.010/recordGap,outputGain=.75;
double field(double tau) noexcept {const double x=speed*(tau+transit/2);return (std::atan((x+recordGap/2)/depth)-std::atan((x-recordGap/2)/depth))/pi;}
void count(uint32_t& n) noexcept {if(n!=std::numeric_limits<uint32_t>::max())++n;}
bool finiteRange(double x,double lo,double hi) noexcept {return std::isfinite(x)&&x>=lo&&x<=hi;}
}
bool TapeSaturation::prepare(float sampleRate,bool weak) noexcept {
    if(!finiteRange(sampleRate,44100,96000))return false;
    TapeSaturation next;next.rate_=sampleRate;next.weak_=weak;const double internal=next.internalRate();
    model::prepareCDF(next.cdf_,next.slope_);double weight=0,remanence=0;
    for(std::size_t r=0;r<kRows;++r){const double x=(model::radius[r]-25500)/6000;next.weights_[r]=std::exp(-.5*x*x);weight+=next.weights_[r];}
    for(std::size_t r=0;r<kRows;++r){next.weights_[r]/=weight;remanence+=next.weights_[r]*std::erf(model::radius[r]/(std::sqrt(2.)*model::sigma));}
    next.fluxUnit_=.145/remanence*16e-6;
    const auto add=[&](std::size_t q,double tau){auto& n=next.counts_[q];if(n==kEvents)return false;
        const double delay=-tau*internal;auto& e=next.events_[q][n++];e.lag=uint32_t(delay);e.fraction=float(delay-e.lag);
        if(e.lag+1>=kHistory)return false;
        const double k=field(tau),phase=2*pi*(q+.5)/kPhases;
        e.audio=float(k*audioDeep);e.bias=float(k*(.6*300*next.biasAmps()/recordGap)*std::sin(2*pi*biasHz*tau+phase));return true;};
    // Stopped fine-grid peak search, with parabolic location refinement. The
    // callback uses only the bounded prepared events; audio-dependent displacement
    // of extrema is an approximation checked against the resolved full carrier.
    constexpr unsigned grid=5376;const double dt=transit/grid;
    for(std::size_t q=0;q<kPhases/2;++q){std::array<double,kEvents> times{};std::size_t n=0;times[n++]=-transit;times[n++]=-transit/2;times[n++]=0;
        const double ph=2*pi*(q+.5)/kPhases;
        const auto b=[&](double t){return field(t)*std::sin(2*pi*biasHz*t+ph);};
        double before=b(-transit),middle=b(-transit+dt);
        for(unsigned j=1;j<grid;++j){const double after=b(-transit+(j+1)*dt);
            if((middle-before)*(after-middle)<0){if(n==kEvents)return false;const double denominator=before-2*middle+after;
                const double offset=denominator==0?0:std::clamp(.5*(before-after)/denominator,-1.,1.);times[n++]=-transit+(j+offset)*dt;}
            before=middle;middle=after;}
        std::sort(times.begin(),times.begin()+n);
        for(std::size_t i=0;i<n;++i)if(!add(q,times[i]))return false;
        // Complete the finite-fringe model with the exterior H=0. Without this
        // final return, extreme drive would report in-field M as remanence.
        // Residual weak fringe cycles beyond40um are an explicit approximation.
        if(next.counts_[q]==kEvents)return false;
        next.events_[q][next.counts_[q]++]={};
        // Exact half-cycle symmetry of the bias field. This is quadrature pairing,
        // not output DC correction; all geometry/audio coefficients are identical.
        next.counts_[q+kPhases/2]=next.counts_[q];next.events_[q+kPhases/2]=next.events_[q];
        for(auto& e:next.events_[q+kPhases/2])e.bias=-e.bias;
    }
    // Physical pickup magnitude from reciprocity: gap sinc * spacing exp *
    // uniform-coating thickness loss. Truncated real inverse Fourier kernel,
    // symmetric/causally shifted; no point-sampling alias of a narrow head gap.
    constexpr unsigned bins=2048;std::array<double,bins/2+1> response{};
    for(unsigned j=0;j<=bins/2;++j){const double f=next.rate_*j/bins,k=2*pi*f/speed,u=k*16e-6,x=k*3e-6/2;
        response[j]=(j?std::sin(x)/x:1)*std::exp(-k*.3e-6)*(j?-std::expm1(-u)/u:1);}
    double headSum=0;
    for(std::size_t i=0;i<=kHeadTaps/2;++i){const double n=double(i)-kHeadTaps/2;double c=response[0]+response[bins/2]*std::cos(pi*n);
        for(unsigned j=1;j<bins/2;++j)c+=2*response[j]*std::cos(2*pi*j*n/bins);
        next.head_[i]=next.head_[kHeadTaps-1-i]=float(c/bins);headSum+=next.head_[i]*(i==kHeadTaps/2?1:2);}
    if(!finiteRange(headSum,.5,1.5))return false;
    for(auto& v:next.head_)v=float(v/headSum);
    double firSum=0;
    for(std::size_t i=0;i<next.fir_.size();++i){const double n=double(i)-(kTaps-1)/2.,cutoff=.4/kFactor;
        next.fir_[i]=float((n==0?2*cutoff:std::sin(2*pi*cutoff*n)/(pi*n))*(.42-.5*std::cos(2*pi*i/(kTaps-1))+.08*std::cos(4*pi*i/(kTaps-1))));firSum+=next.fir_[i]*(i+1==next.fir_.size()?1:2);}
    for(auto& v:next.fir_)v=float(v/firSum);
    const double c=2*internal;next.b0_=(1+c*50e-6)/(1+c*5e-6);next.b1_=(1-c*50e-6)/(1+c*5e-6);next.a1_=(1-c*5e-6)/(1+c*5e-6);
    const double g=std::tan(pi*30/next.rate_);next.hpG_=g/(1+g);
    next.prepared_=true;next.reset();*this=next;return true;
}
void TapeSaturation::reset() noexcept {
    history_.fill(0);up_.fill(0);down_.fill(0);headHistory_.fill(0);
    historyIndex_=upIndex_=downIndex_=headIndex_=0;recordX_=recordY_=hpState_=previousHead_=0;
    failures_=invalid_=inputClamps_=outputClamps_=parcels_=0;probe_={};running_=prepared_;
}
double TapeSaturation::parcel(std::size_t q,TapeParcelTrace* trace) const noexcept {
    std::array<double,kRows> p{};double m=0;if(trace)trace->count=counts_[q];
    for(std::size_t k=0;k<counts_[q];++k){const auto& e=events_[q][k];const auto i=(historyIndex_+e.lag)%kHistory,j=(i+1)%kHistory;
        const double v=history_[i]+double(e.fraction)*(history_[j]-history_[i]),h=e.audio*v+e.bias;
        for(std::size_t r=0;r<kRows;++r)p[r]=model::advance(p[r],h,model::radius[r]);
        if(trace){auto& t=trace->points[k];t.relativeSeconds=-(e.lag+double(e.fraction))/internalRate();t.field_A_per_m=h;t.boundary_A_per_m=p;t.magnetization=0;
            for(std::size_t r=0;r<kRows;++r)t.magnetization+=weights_[r]*model::gaussian(p[r]/model::sigma,cdf_,slope_);}
    }
    for(std::size_t r=0;r<kRows;++r)m+=weights_[r]*model::gaussian(p[r]/model::sigma,cdf_,slope_);
    return m;
}
double TapeSaturation::material(float recordedVolts) noexcept {
    historyIndex_=historyIndex_==0?kHistory-1:historyIndex_-1;history_[historyIndex_]=recordedVolts;
    double m=0;for(std::size_t q=0;q<kPhases/2;++q)m+=(parcel(q,nullptr)+parcel(q+kPhases/2,nullptr))/kPhases;
    count(parcels_);probe_.recordedVolts=recordedVolts;probe_.remanence=m;probe_.flux_nWb_per_m=m*fluxUnit_*1e9;return m;
}
float TapeSaturation::materialReference(float v) noexcept {
    if(!running_)return 0;
    if(!std::isfinite(v)){count(invalid_);return 0;}
    if(std::abs(v)>64){v=std::clamp(v,-64.f,64.f);count(inputClamps_);}return float(material(v));
}
bool TapeSaturation::traceParcel(std::size_t q,TapeParcelTrace& t) const noexcept {
    if(!prepared_||q>=kPhases)return false;
    parcel(q,&t);return true;
}
float TapeSaturation::process(float input) noexcept {
    if(!running_)return 0;
    if(!std::isfinite(input)){count(invalid_);return 0;}
    if(std::abs(input)>2){input=std::clamp(input,-2.f,2.f);count(inputClamps_);}
    upIndex_=upIndex_==0?uint32_t(up_.size()-1):upIndex_-1;up_[upIndex_]=input;double decimated=0;
    for(std::size_t phase=0;phase<kFactor;++phase){float interpolated=0;auto i=upIndex_;
        for(std::size_t tap=phase;tap<kTaps;tap+=kFactor){interpolated+=coefficient(tap)*up_[i];if(++i==up_.size())i=0;}
        const double x=kFactor*interpolated,recorded=b0_*x+b1_*recordX_-a1_*recordY_;recordX_=x;recordY_=recorded;
        const double m=material(float(recorded));downIndex_=downIndex_==0?kTaps-1:downIndex_-1;down_[downIndex_]=float(m);
        if(phase==0){i=downIndex_;for(std::size_t tap=0;tap<kTaps;++tap){decimated+=coefficient(tap)*down_[i];if(++i==kTaps)i=0;}}
    }
    headIndex_=headIndex_==0?kHeadTaps-1:headIndex_-1;headHistory_[headIndex_]=float(decimated);double head=0;auto i=headIndex_;
    for(std::size_t tap=0;tap<kHeadTaps;++tap){head+=head_[tap]*headHistory_[i];if(++i==kHeadTaps)i=0;}
    // Induction and flux-integrating electronics analytically combine into a
    // leaky highpass. Coil diagnostic is a labeled backward-difference voltage.
    probe_.headRemanence=head;probe_.coilVolts=-300*.6*.002575*fluxUnit_*rate_*(head-previousHead_);previousHead_=head;
    const double v=(head-hpState_)*hpG_,low=v+hpState_;hpState_=low+v;
    probe_.outputVolts=outputGain*(head-low);double result=probe_.outputVolts/8;
    if(!std::isfinite(result)){count(failures_);return 0;}
    if(std::abs(result)>1){count(outputClamps_);result=std::clamp(result,-1.,1.);}return float(result);
}
} // namespace parable::dsp
