#include "parable_cmos.h"
#include "cmos_device.h"
#include <algorithm>
#include <cmath>

namespace parable::dsp {
namespace {
constexpr double pi=3.14159265358979323846;
bool between(double x,double lo,double hi) noexcept {return std::isfinite(x)&&x>=lo&&x<=hi;}
double tidy(double x) noexcept {return std::abs(x)<1e-20?0:x;}
bool valid(const CmosCircuit& c) noexcept {
    return between(c.railVolts,6,9)&&between(c.inputOhms,50e3,200e3)
        &&between(c.inputFarads,10e-9,220e-9)&&between(c.firstFeedbackOhms,50e3,200e3)
        &&between(c.drivePotOhms,100e3,1e6)&&between(c.firstFeedbackFarads,25e-12,200e-12)
        &&between(c.interstageFarads,330e-12,100e-9)&&between(c.secondFeedbackOhms,100e3,2e6)
        &&between(c.secondFeedbackFarads,50e-12,220e-12)&&between(c.outputFarads,1e-6,22e-6)
        &&between(c.volumePotOhms,5e3,20e3)&&between(c.loadOhms,100e3,2e6)
        &&between(c.secondBiasSiemens,0,1./100e3);
}
}
Cmos::Cmos() noexcept {configure(48000);}
bool Cmos::configure(float sampleRate,CmosControls controls,const CmosCircuit& c,CmosRate mode) noexcept {
    if(!between(sampleRate,32000,96000)||!between(controls.drive,0,1)||!between(controls.volume,0,1)
        ||!valid(c)||(mode!=CmosRate::Reference1x&&mode!=CmosRate::Oversampled4x))return false;
    // Off-audio DC self-bias. No arbitrary saturation/bias voice constants.
    double lo=0,hi=c.railVolts;
    for(int i=0;i<64;++i){const double v=(lo+hi)/2;
        if(cmos_detail::inverter(v,v,c.railVolts).value>0)hi=v;else lo=v;}
    const double firstBias=(lo+hi)/2;
    double secondOutput=firstBias,secondGate=firstBias;
    if(c.secondBiasSiemens>0) {
        const double gf=1/c.secondFeedbackOhms;
        const double ratio=gf/(gf+c.secondBiasSiemens);
        lo=0;hi=c.railVolts;
        for(int i=0;i<64;++i) {
            const double out=(lo+hi)/2,gate=ratio*out;
            const double residual=cmos_detail::inverter(gate,out,c.railVolts).value+gf*(out-gate);
            if(residual>0)hi=out;else lo=out;
        }
        secondOutput=(lo+hi)/2;secondGate=ratio*secondOutput;
        // Refuse an invalid prepared operating point before modifying state.
        if(std::abs(cmos_detail::inverter(secondGate,secondOutput,c.railVolts).value
                    +gf*(secondOutput-secondGate))>1e-10)return false;
    }
    circuit_=c;mode_=mode;bias_=firstBias;biasGate2_=secondGate;biasOutput2_=secondOutput;
    rate_=sampleRate*(mode==CmosRate::Oversampled4x?4:1);
    const double k=2*rate_;
    cfin_=k*c.inputFarads;gin_=1/(c.inputOhms+1/cfin_);
    cf1_=k*c.firstFeedbackFarads;gf1_=1/(c.firstFeedbackOhms+controls.drive*c.drivePotOhms)+cf1_;
    link_=k*c.interstageFarads;cf2_=k*c.secondFeedbackFarads;gf2_=1/c.secondFeedbackOhms+cf2_;
    const double bottom=c.volumePotOhms*controls.volume;
    bottom_=bottom*c.loadOhms/(bottom+c.loadOhms);
    rout_=c.volumePotOhms*(1-controls.volume)+bottom_;
    gout_=1/(rout_+1/(k*c.outputFarads));
    a1_=gin_/(gin_+gf1_);b1_=gf1_/(gin_+gf1_);
    a2_=link_/(link_+gf2_+c.secondBiasSiemens);b2_=gf2_/(link_+gf2_+c.secondBiasSiemens);
    // Owned PAR-007 FIR technique reused without changing the protected RAT code.
    double sum=0;
    for(std::size_t i=0;i<kTaps;++i){const double n=double(i)-(kTaps-1)/2.;
        const double h=(n==0?.2:std::sin(2*pi*.1*n)/(pi*n))
            *(.42-.5*std::cos(2*pi*i/(kTaps-1))+.08*std::cos(4*pi*i/(kTaps-1)));
        fir_[i]=static_cast<float>(h);sum+=fir_[i];}
    for(auto& v:fir_)v=static_cast<float>(v/sum);
    reset();return true;
}
void Cmos::resetState() noexcept {
    zInput_=-bias_;zFirst_=0;zLink_=bias_-biasGate2_;zSecond_=biasGate2_-biasOutput2_;zOutput_=biasOutput2_;
    previous1_=bias_;previous2_=biasOutput2_;
    nodes_={bias_,bias_,biasGate2_,biasOutput2_,0,0,0,true};
}
void Cmos::reset() noexcept {
    resetState();up_.fill(0);down_.fill(0);upIndex_=downIndex_=0;failed_=calls_=evaluations_=0;maximum_=0;
}
double Cmos::circuitSample(double x) noexcept {
    const double h1=(-gin_*zInput_+cf1_*zFirst_)/(gin_+gf1_);
    const double h2=(-link_*zLink_+cf2_*zSecond_)/(link_+gf2_+circuit_.secondBiasSiemens);
    double o1=previous1_,o2=previous2_,g1=0,g2=0,f1=0,f2=0;
    unsigned iteration=0;
    // Dedicated 2x2 Newton solve, at most 16 iterations; no general netlist engine.
    for(;iteration<16;++iteration){
        g1=a1_*x+b1_*o1+h1;g2=a2_*o1+b2_*o2+h2;
        const auto m1=cmos_detail::inverter(g1,o1,circuit_.railVolts);
        const auto m2=cmos_detail::inverter(g2,o2,circuit_.railVolts);
        f1=m1.value-gf1_*(g1-o1)+cf1_*zFirst_+link_*(o1-g2-zLink_);
        f2=m2.value-gf2_*(g2-o2)+cf2_*zSecond_+gout_*(o2-zOutput_);
        if(std::max(std::abs(f1),std::abs(f2))<1e-10)break;
        const double j11=m1.gateDerivative*b1_+m1.drainDerivative+gf1_*(1-b1_)+link_*(1-a2_);
        const double j12=-link_*b2_,j21=a2_*(m2.gateDerivative-gf2_);
        const double j22=m2.gateDerivative*b2_+m2.drainDerivative+gf2_*(1-b2_)+gout_;
        const double determinant=j11*j22-j12*j21;
        if(!std::isfinite(determinant)||determinant<=0)break;
        const double d1=(j22*f1-j12*f2)/determinant,d2=(j11*f2-j21*f1)/determinant;
        const double scale=std::max(1.,std::max(std::abs(d1),std::abs(d2))/(circuit_.railVolts/4));
        o1=std::clamp(o1-d1/scale,0.,circuit_.railVolts);
        o2=std::clamp(o2-d2/scale,0.,circuit_.railVolts);
    }
    // Recompute the residual at the actual final iterate, including budget expiry.
    g1=a1_*x+b1_*o1+h1;g2=a2_*o1+b2_*o2+h2;
    f1=cmos_detail::inverter(g1,o1,circuit_.railVolts).value-gf1_*(g1-o1)+cf1_*zFirst_+link_*(o1-g2-zLink_);
    f2=cmos_detail::inverter(g2,o2,circuit_.railVolts).value-gf2_*(g2-o2)+cf2_*zSecond_+gout_*(o2-zOutput_);
    const double residual=std::max(std::abs(f1),std::abs(f2));
    const unsigned loops=std::min(iteration+1,16u);
    ++calls_;evaluations_+=loops;maximum_=std::max(maximum_,loops);
    if(!std::isfinite(residual)||residual>1e-9){++failed_;resetState();nodes_.converged=false;nodes_.residualAmps=residual;return 0;}
    const double inputCurrent=gin_*(x-g1-zInput_),outputCurrent=gout_*(o2-zOutput_);
    const double jack=outputCurrent*bottom_;
    zInput_=tidy(2*(x-g1-circuit_.inputOhms*inputCurrent)-zInput_);
    zFirst_=tidy(2*(g1-o1)-zFirst_);zLink_=tidy(2*(o1-g2)-zLink_);
    zSecond_=tidy(2*(g2-o2)-zSecond_);zOutput_=tidy(2*(o2-rout_*outputCurrent)-zOutput_);
    previous1_=o1;previous2_=o2;
    nodes_={g1,o1,g2,o2,jack,residual,loops,true};
    return jack/4;
}
float Cmos::process(float input) noexcept {
    if(!std::isfinite(input))return 0;
    const float x=static_cast<float>(tidy(std::clamp(double(input),-2.,2.)));double result=0;
    if(mode_==CmosRate::Reference1x)result=circuitSample(x);
    else{
        upIndex_=upIndex_==0?up_.size()-1:upIndex_-1;up_[upIndex_]=x;
        for(std::size_t phase=0;phase<4;++phase){float interpolated=0;auto index=upIndex_;
            for(std::size_t tap=phase;tap<kTaps;tap+=4){interpolated+=fir_[tap]*up_[index];if(++index==up_.size())index=0;}
            downIndex_=downIndex_==0?kTaps-1:downIndex_-1;
            down_[downIndex_]=static_cast<float>(tidy(circuitSample(interpolated*4)));
            if(phase==0){index=downIndex_;for(std::size_t tap=0;tap<kTaps;++tap){result+=fir_[tap]*down_[index];if(++index==kTaps)index=0;}}
        }
    }
    if(!std::isfinite(result)){resetState();++failed_;return 0;}
    return static_cast<float>(tidy(std::clamp(result,-1.,1.)));
}
void Cmos::processBlock(const float* input,float* output,std::size_t count) noexcept {
    for(std::size_t i=0;i<count;++i)output[i]=process(input[i]);
}
} // namespace parable::dsp
