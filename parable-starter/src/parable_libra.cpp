#include "parable_libra.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace parable::dsp {
namespace {constexpr double pi=3.14159265358979323846;constexpr float A=40,rail=3.5f,vbe=.64f;bool unit(float v){return std::isfinite(v)&&v>=0&&v<=1;}void count(uint32_t& v){if(v!=std::numeric_limits<uint32_t>::max())++v;}}
LibraProbe Libra::select(float drive,bool steady) const noexcept {
    for(const auto& r:regions_){const float raw=drive*(steady?r.dcInv:r.dynamicInv)+(steady?r.dcOffset:r.dynamicOffset),u=std::clamp(raw,-rail,rail),y=r.m*u+r.c;
        const float en=aN_*u+bN_-y-vbe,ep=y-aP_*u+bP_-vbe;constexpr float epsilon=2e-6f;
        if((r.mask&1?en>=-epsilon:en<=epsilon)&&(r.mask&2?ep>=-epsilon:ep<=epsilon))return {u,y,gN_*std::max(en,0.f),gP_*std::max(ep,0.f),0,0,0,u!=raw,true};
    }return {};
}
bool Libra::prepare(float rate,LibraControls c,unsigned factor,bool feedback,float loadOhms) noexcept {
    if(!std::isfinite(rate)||rate<44100||rate>96000||!unit(c.drive)||!unit(c.level)||(factor!=1&&factor!=2&&factor!=4)||!std::isfinite(loadOhms)||loadOhms<100||loadOhms>10000)return false;
    Libra next;next.controls_=c;next.factor_=factor;next.taps_=24*factor+1;const double fs=rate*factor,rb1=c.damagedBias?270000:27000,rb2=4700,rb4=27000,rb3=4700;
    next.aN_=float(rb1/(rb1+rb2));next.bN_=float(4.5*rb2/(rb1+rb2));next.aP_=float(rb4/(rb4+rb3));next.bP_=float(4.5*rb3/(rb4+rb3));
    next.gN_=float(1/(2.2+(rb1*rb2/(rb1+rb2))/101));next.gP_=float(1/(2.2+(rb4*rb3/(rb4+rb3))/101));next.load_=float(1/(1/double(loadOhms)+1./40000));next.beta_=feedback?.25f:0;next.k_=float(2*pi*500/fs);next.gain_=float(4*std::pow(20.,c.drive));
    const unsigned masks[]={3,1,2,0};for(unsigned n=0;n<4;++n){auto& r=next.regions_[n];r.mask=masks[n];const float gn=r.mask&1?next.gN_:0,gp=r.mask&2?next.gP_:0,den=1/next.load_+gn+gp;
        r.m=(gn*next.aN_+gp*next.aP_)/den;r.c=(gn*(next.bN_-vbe)-gp*(next.bP_-vbe))/den;
        r.dynamicInv=1/(1+next.k_+next.k_*A*next.beta_*r.m);r.dynamicOffset=-next.k_*A*next.beta_*r.c*r.dynamicInv;
        r.dcInv=1/(1+A*next.beta_*r.m);r.dcOffset=-A*next.beta_*r.c*r.dcInv;}
    next.bias_=next.select(0,true);if(!next.bias_.valid)return false;
    next.inputG_=float(1/(1+2*fs*.01));next.outputG_=float(1/(1+2*fs*.1));
    const unsigned center=(next.taps_-1)/2;double sum=0;for(unsigned n=0;n<=center;++n){const double x=double(n)-center,cutoff=.4/factor;next.fir_[n]=float((x==0?2*cutoff:std::sin(2*pi*cutoff*x)/(pi*x))*(.42-.5*std::cos(2*pi*n/(next.taps_-1))+.08*std::cos(4*pi*n/(next.taps_-1))));sum+=next.fir_[n]*(n==center?1:2);}for(auto& x:next.fir_)x=float(x/sum);
    next.prepared_=true;next.reset();*this=next;return true;
}
LibraProbe Libra::dc(float input) const noexcept {if(!prepared_||!std::isfinite(input))return {};auto p=select(A*std::clamp(input,-4.f,4.f),true);p.errorVolts=input-beta_*p.combinedVolts;p.poleResidualVolts=p.driverVolts-A*p.errorVolts;return p;}
void Libra::reset() noexcept {up_.fill(0);down_.fill(0);upIndex_=downIndex_=0;inputZ_=outputZ_=0;driver_=bias_.driverVolts;failures_=clamps_=invalid_=rails_=0;probe_=bias_;running_=prepared_;}
float Libra::sample(float input) noexcept {
    const float v=(input-inputZ_)*inputG_,low=inputZ_+v;inputZ_=low+v;const float pre=std::clamp((input-low)*gain_,-4.f,4.f),previous=driver_;
    auto p=select(previous+k_*A*pre,false); // implicit feedback uses this sample's combined output
    if(!p.valid){count(failures_);return 0;}
    driver_=p.driverVolts;p.errorVolts=pre-beta_*p.combinedVolts;p.poleResidualVolts=driver_-previous-k_*(A*p.errorVolts-driver_);if(p.railed)count(rails_);
    const float ac=p.combinedVolts-bias_.combinedVolts,w=(ac-outputZ_)*outputG_,l=outputZ_+w;outputZ_=l+w;p.jackVolts=controls_.level*(ac-l);probe_=p;return p.jackVolts/4;
}
float Libra::process(float input) noexcept {
    if(!running_)return 0;
    if(!std::isfinite(input)){count(invalid_);return 0;}
    if(std::abs(input)>.5f){count(clamps_);input=std::clamp(input,-.5f,.5f);}
    float y=0;if(factor_==1)y=sample(input);else {upIndex_=upIndex_?upIndex_-1:24;up_[upIndex_]=input;
        for(unsigned phase=0;phase<factor_;++phase){float x=0;unsigned j=upIndex_;for(unsigned tap=phase;tap<taps_;tap+=factor_){x+=coefficient(tap)*up_[j];if(++j==25)j=0;}
            downIndex_=downIndex_?downIndex_-1:taps_-1;down_[downIndex_]=sample(float(factor_)*x);if(phase==0){j=downIndex_;for(unsigned tap=0;tap<taps_;++tap){y+=coefficient(tap)*down_[j];if(++j==taps_)j=0;}}}}
    if(!std::isfinite(y)||std::abs(y)>1){count(failures_);return std::isfinite(y)?std::clamp(y,-1.f,1.f):0;}return y;
}
}
