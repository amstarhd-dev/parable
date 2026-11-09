#include "parable_rat.h"
#include "rat_circuit_math.h"
#include <algorithm>
#include <cmath>
namespace parable::dsp {
namespace {
constexpr double pi=3.14159265358979323846;
double tidy(double x) noexcept {return std::abs(x)<1e-20 ? 0.:x;}
bool between(double x,double lo,double hi) noexcept {return std::isfinite(x)&&x>=lo&&x<=hi;}
bool valid(const RatCircuit& c) noexcept {
    const double resistors[]={c.inputBiasOhms,c.inputSeriesOhms,c.feedbackBassOhms,c.feedbackTrebleOhms,
        c.distortionPotOhms,c.clippingSeriesOhms,c.filterFixedOhms,c.filterPotOhms,c.gateOhms,
        c.outputSeriesOhms,c.volumePotOhms,c.loadOhms};
    for(double r:resistors)if(!between(r,10,1e7))return false;
    const double caps[]={c.inputCapFarads,c.feedbackBassFarads,c.feedbackTrebleFarads,c.couplingFarads,
        c.gateCapFarads,c.outputCapFarads};
    for(double v:caps)if(!between(v,1e-9,1e-3))return false;
    return between(c.inputShuntFarads,1e-10,1e-7)&&between(c.feedbackCompFarads,1e-11,2e-7)
        &&between(c.filterFarads,1e-9,1e-7)&&between(c.opampOpenLoopGain,1e4,1e6)
        &&between(c.opampGbwHz,1e5,3e6)&&between(c.opampSlewVoltsPerUs,.01,1.)
        &&between(c.supplyVolts,5,12)&&between(c.railMarginVolts,.1,2)
        &&c.supplyVolts/2-c.railMarginVolts>=.5
        &&between(c.diodePositiveIsAmps,1e-12,1e-6)&&between(c.diodeNegativeIsAmps,1e-12,1e-6)
        &&between(c.diodeIdeality,1.5,2.5)&&between(c.thermalVolts,.02,.04);
}
bool stable(rat_detail::Filter f) noexcept {
    // Schur recursion, control-side only. Reject numerically unstable coefficients.
    auto a=f.a;
    for(unsigned n=f.order;n>0;--n){
        const double k=a[n];if(!std::isfinite(k)||std::abs(k)>=1)return false;
        auto old=a;for(unsigned i=0;i<n;++i)a[i]=(old[i]-k*old[n-i])/(1-k*k);
    }
    return true;
}
}
namespace rat_detail {
void Filter::prepare(const Polynomial& numerator,const Polynomial& denominator,double rate) noexcept {
    order=4;while(order>0&&numerator[order]==0&&denominator[order]==0)--order;
    bilinear(numerator,denominator,order,rate,b,a);reset();
}
double Filter::process(double v) noexcept {
    for(unsigned i=order;i>0;--i){x[i]=x[i-1];y[i]=y[i-1];}
    x[0]=v;double out=0;
    for(unsigned i=0;i<=order;++i)out+=b[i]*x[i];
    for(unsigned i=1;i<=order;++i)out-=a[i]*y[i];
    y[0]=tidy(out);return y[0];
}
void ClipStage::prepare(double rate,RatControls p,const RatCircuit& c) noexcept {
    const double k=2*rate;
    couplingR=1/(k*c.couplingFarads);clipR=c.clippingSeriesOhms;
    sourceG=1/(clipR+couplingR);
    filterG=1/(c.filterFixedOhms+c.filterPotOhms*pot(p.filter));
    toneG=k*c.filterFarads;gateR=c.gateOhms;
    gateG=1/(gateR+1/(k*c.gateCapFarads));
    toneInv=1/(filterG+toneG+gateG);toneInput=filterG*toneInv;
    portR=1/(sourceG+filterG-filterG*toneInput);
    const double bottom=c.volumePotOhms*p.volume;
    bottomR=bottom*c.loadOhms/(bottom+c.loadOhms);
    outR=c.outputSeriesOhms+c.volumePotOhms*(1-p.volume)+bottomR;
    outG=1/(outR+1/(k*c.outputCapFarads));
    const double nv=c.diodeIdeality*c.thermalVolts;
    for(std::size_t i=0;i<kTable;++i){
        const double input=-16.+i/64.;double lo=-16,hi=16;
        for(int iteration=0;iteration<56;++iteration){
            const double v=(lo+hi)/2;
            const double current=(c.positiveDiodeConnected ? c.diodePositiveIsAmps*std::expm1(v/nv):0)
                -(c.negativeDiodeConnected ? c.diodeNegativeIsAmps*std::expm1(-v/nv):0);
            if(v+portR*current<input)lo=v;else hi=v;
        }
        inverse[i]=static_cast<float>((lo+hi)/2);
    }
    inverse[(kTable-1)/2]=0;reset();
}
double ClipStage::lookup(double x) const noexcept {
    const double position=std::abs(std::clamp(x,-16.,16.))*64.;
    const auto i=static_cast<std::size_t>(position);constexpr auto mid=(kTable-1)/2;
    if(i>=mid)return x<0?inverse.front():inverse.back();
    const auto index=x<0 ? mid-i:mid+i;const auto next=x<0?index-1:index+1;
    return inverse[index]+(position-i)*(double(inverse[next])-inverse[index]);
}
double ClipStage::process(double source) noexcept {
    const double history=(toneG*zTone+gateG*zGate)*toneInv;
    const double v=lookup(portR*(sourceG*(source-zCoupling)+filterG*history));
    const double t=toneInput*v+history;
    const double current=sourceG*(source-v-zCoupling);
    zCoupling=tidy(2*(source-v-clipR*current)-zCoupling);
    zTone=tidy(2*t-zTone);
    const double gate=gateR*gateG*(t-zGate);
    zGate=tidy(2*(t-gate)-zGate);
    const double outputCurrent=outG*(gate-zOut);
    zOut=tidy(2*(gate-outR*outputCurrent)-zOut);
    clip=static_cast<float>(v);tone=static_cast<float>(t);
    return tidy(outputCurrent*bottomR);
}
bool preparePreamp(Filter& input,Filter& gain,double fs,RatControls p,const RatCircuit& c) noexcept {
    Polynomial b={0,c.inputCapFarads*c.inputBiasOhms,0,0,0};
    Polynomial a={1,c.inputCapFarads*c.inputBiasOhms+c.inputShuntFarads*(c.inputBiasOhms+c.inputSeriesOhms),
        c.inputCapFarads*c.inputBiasOhms*c.inputSeriesOhms*c.inputShuntFarads,0,0};
    input.prepare(b,a,fs);
    feedbackPolynomial(c.feedbackBassOhms,c.feedbackTrebleOhms,c.feedbackBassFarads,
        c.trebleLegConnected?c.feedbackTrebleFarads:0,c.distortionPotOhms*pot(p.distortion),c.feedbackCompFarads,b,a);
    // Finite A extension: A0*N / (A0*D + N*(1+s/wp)).
    Polynomial finiteB{},finiteA{};
    const double wp=2*pi*c.opampGbwHz/c.opampOpenLoopGain;
    for(unsigned i=0;i<5;++i){finiteB[i]=c.opampOpenLoopGain*b[i];
        finiteA[i]=c.opampOpenLoopGain*a[i]+b[i]+(i?b[i-1]/wp:0);}
    gain.prepare(finiteB,finiteA,fs);
    return stable(input)&&stable(gain);
}

}
Rat::Rat() noexcept {configure(48000);}
bool Rat::configure(float rate,RatControls p,const RatCircuit& c,RatRate mode) noexcept {
    if(!between(rate,32000,96000)||!between(p.distortion,0,1)||!between(p.filter,0,1)
        ||!between(p.volume,0,1)||!valid(c)||(mode!=RatRate::Reference1x&&mode!=RatRate::Oversampled4x))return false;
    const double fs=rate*(mode==RatRate::Oversampled4x ? 4:1);
    rat_detail::Filter input,gain;
    if(!rat_detail::preparePreamp(input,gain,fs,p,c))return false;
    input_=input;gain_=gain;controls_=p;circuit_=c;mode_=mode;
    rail_=c.supplyVolts/2-c.railMarginVolts;slewStep_=c.opampSlewVoltsPerUs*1e6/fs;
    clipper_.prepare(fs,p,c);
    double sum=0;
    for(std::size_t i=0;i<kTaps;++i){const double n=double(i)-(kTaps-1)/2.;
        const double h=(n==0?.2:std::sin(2*pi*.1*n)/(pi*n))
            *(.42-.5*std::cos(2*pi*i/(kTaps-1))+.08*std::cos(4*pi*i/(kTaps-1)));
        fir_[i]=static_cast<float>(h);sum+=fir_[i];}
    for(auto& v:fir_)v=static_cast<float>(v/sum);
    reset();return true;
}
void Rat::reset() noexcept {
    input_.reset();gain_.reset();clipper_.reset();up_.fill(0);down_.fill(0);
    upIndex_=downIndex_=0;previousOpamp_=0;nodes_={};
}
double Rat::circuitSample(double input) noexcept {
    const double linear=gain_.process(input_.process(input));
    if(!std::isfinite(linear)||std::abs(linear)>1e6){reset();return 0;}
    const double target=std::clamp(linear,-rail_,rail_);
    const double opamp=previousOpamp_+std::clamp(target-previousOpamp_,-slewStep_,slewStep_);
    previousOpamp_=tidy(opamp);
    const double jack=clipper_.process(opamp);
    nodes_={static_cast<float>(linear),static_cast<float>(opamp),clipper_.clip,clipper_.tone,static_cast<float>(jack)};
    return jack/4.;
}
float Rat::process(float input) noexcept {
    if(!std::isfinite(input))return 0;
    const float x=static_cast<float>(tidy(std::clamp(double(input),-4.,4.)));double result=0;
    if(mode_==RatRate::Reference1x)result=circuitSample(x);
    else {
        upIndex_=upIndex_==0?up_.size()-1:upIndex_-1;up_[upIndex_]=x;
        for(std::size_t phase=0;phase<4;++phase){float interpolated=0;auto index=upIndex_;
            for(std::size_t tap=phase;tap<kTaps;tap+=4){interpolated+=fir_[tap]*up_[index];if(++index==up_.size())index=0;}
            downIndex_=downIndex_==0?kTaps-1:downIndex_-1;
            down_[downIndex_]=static_cast<float>(tidy(circuitSample(interpolated*4.)));
            if(phase==0){index=downIndex_;for(std::size_t tap=0;tap<kTaps;++tap){
                result+=fir_[tap]*down_[index];if(++index==kTaps)index=0;}}
        }
    }
    if(!std::isfinite(result)){reset();return 0;}
    return static_cast<float>(tidy(std::clamp(result,-1.,1.)));
}
void Rat::processBlock(const float* input,float* output,std::size_t count) noexcept {
    for(std::size_t i=0;i<count;++i)output[i]=process(input[i]);
}
} // namespace parable::dsp
