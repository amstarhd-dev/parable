#include "parable_germanium.h"
#include "germanium_device_select.h"
#include <algorithm>
#include <cmath>

namespace parable::dsp {
namespace {
constexpr double pi=3.14159265358979323846;
bool range(double x,double lo,double hi) noexcept {return std::isfinite(x)&&x>=lo&&x<=hi;}
double tidy(double x) noexcept {return std::abs(x)<1e-20?0:x;}
bool validDevice(const GermaniumDevice& p) noexcept {
    return range(p.forwardGain,40,300)&&range(p.reverseGain,3,25)
        &&range(p.saturationAmps,10e-6,30e-6)&&range(p.beLeakageAmps,0,25e-6)
        &&range(p.bcLeakageAmps,0,100e-6);
}
bool valid(const GermaniumCircuit& c) noexcept {
    return range(c.supplyVolts,6,9)&&range(c.sourceOhms,100,100e3)
        &&range(c.inputFarads,1e-6,4.7e-6)&&range(c.collector1Ohms,20e3,50e3)
        &&range(c.feedbackOhms,10e3,150e3)&&range(c.fuzzOhms,500,2000)
        &&range(c.bypassFarads,10e-6,47e-6)&&range(c.collector2Ohms,4.7e3,22e3)
        &&range(c.supplyTapOhms,220,1000)&&range(c.outputFarads,5e-9,22e-9)
        &&range(c.volumeOhms,250e3,1e6)&&range(c.loadOhms,100e3,2e6)
        &&validDevice(c.q1)&&validDevice(c.q2);
}
}
Germanium::Germanium() noexcept {configure(48000);}
void Germanium::coefficients(bool dc) noexcept {
    const auto& c=circuit_;const double k=dc?0:2*rate_;
    gcapIn_=k*c.inputFarads;gin_=dc?0:1/(c.sourceOhms+1/gcapIn_);
    gcapFuzz_=k*c.bypassFarads;
    const double bottom=c.volumeOhms*controls_.volume;
    bottom_=bottom*c.loadOhms/(bottom+c.loadOhms);
    volumeR_=c.volumeOhms*(1-controls_.volume)+bottom_;
    gcapOut_=dc?0:1/(volumeR_+1/(k*c.outputFarads));
    const double top=c.fuzzOhms*(1-controls_.fuzz),low=c.fuzzOhms*controls_.fuzz;
    if(low==0){emitG_=1/top;emitHistory_=wiperA_=wiperZ_=0;}
    else{const double denominator=1+top*(1/low+gcapFuzz_);
        emitG_=(1/low+gcapFuzz_)/denominator;emitHistory_=-gcapFuzz_/denominator;
        wiperA_=1/denominator;wiperZ_=top*gcapFuzz_/denominator;}
    const double g2=1/c.collector2Ohms,gs=1/c.supplyTapOhms,den=g2+gs+gcapOut_;
    tapA_=g2/den;tapSupply_=gs*c.supplyVolts/den;tapHistory_=gcapOut_/den;
    collectorG_=g2*(1-tapA_);
    passive_={};const double feedback=1/c.feedbackOhms;
    passive_[0][0]=gin_+feedback;passive_[0][2]=-feedback;
    passive_[1][1]=1/c.collector1Ohms;
    passive_[2][0]=-feedback;passive_[2][2]=feedback+emitG_;passive_[3][3]=collectorG_;
    const double bc=k*100e-12;
    for(auto pair:{std::array<unsigned,2>{0,1},std::array<unsigned,2>{1,3}}){
        const auto a=pair[0],b=pair[1];passive_[a][a]+=bc;passive_[b][b]+=bc;
        passive_[a][b]-=bc;passive_[b][a]-=bc;}
}
bool Germanium::configure(float rate,GermaniumControls controls,const GermaniumCircuit& c,GermaniumRate mode) noexcept {
    if(!range(rate,32000,mode==GermaniumRate::Reference1x?192000:96000)||!range(controls.fuzz,0,1)||!range(controls.volume,0,1)
        ||!valid(c)||(mode!=GermaniumRate::Reference1x&&mode!=GermaniumRate::Oversampled4x))return false;
    Germanium next(false);next.circuit_=c;next.controls_=controls;next.mode_=mode;
    next.rate_=rate*(mode==GermaniumRate::Oversampled4x?4:1);next.coefficients(true);
    Vector x{.073,.21,.16,7.55};const Vector rhs{0,c.supplyVolts/c.collector1Ohms,0,next.tapSupply_/c.collector2Ohms};
    double residual=0;unsigned loops=0;
    if(!next.solve(x,rhs,next.passive_,120,residual,loops))return false;
    next.bias_=x;next.biasNodes_={-x[0],-x[1],-x[2],-x[3],-(next.tapA_*x[3]+next.tapSupply_),0,0,residual,loops,true};
    next.coefficients(false);
    // Owned PAR-007/SF-002 FIR technique; previous kernels remain untouched.
    double sum=0;
    for(std::size_t i=0;i<kTaps;++i){const double n=double(i)-(kTaps-1)/2.;
        const double h=(n==0?.2:std::sin(2*pi*.1*n)/(pi*n))
            *(.42-.5*std::cos(2*pi*i/(kTaps-1))+.08*std::cos(4*pi*i/(kTaps-1)));
        next.fir_[i]=static_cast<float>(h);sum+=next.fir_[i];}
    for(auto& v:next.fir_)v=static_cast<float>(v/sum);
    next.reset();*this=next;return true;
}
bool Germanium::solve(Vector& x,const Vector& rhs,const Matrix& passive,unsigned budget,double& residual,unsigned& loops) const noexcept {
    // Four fixed circuit nodes, not a general netlist solver. x=-physical volts.
    for(loops=0;loops<=budget;++loops){
        Matrix j=passive;Vector f{};
        for(unsigned a=0;a<4;++a){f[a]=-rhs[a];for(unsigned b=0;b<4;++b)f[a]+=passive[a][b]*x[b];}
        const auto q1=germanium_detail::deviceSelected(x[0],x[0]-x[1],circuit_.q1);
        const auto q2=germanium_detail::deviceSelected(x[1]-x[2],x[1]-x[3],circuit_.q2);
        if(!q1.valid||!q2.valid)return false;
        f[0]+=q1.base;f[1]+=q1.collector+q2.base;f[2]-=q2.collector+q2.base;f[3]+=q2.collector;
        j[0][0]+=q1.baseE+q1.baseC;j[0][1]-=q1.baseC;
        j[1][0]+=q1.collectorE+q1.collectorC;j[1][1]-=q1.collectorC;
        j[1][1]+=q2.baseE+q2.baseC;j[1][2]-=q2.baseE;j[1][3]-=q2.baseC;
        j[2][1]-=q2.collectorE+q2.collectorC+q2.baseE+q2.baseC;
        j[2][2]+=q2.collectorE+q2.baseE;j[2][3]+=q2.collectorC+q2.baseC;
        j[3][1]+=q2.collectorE+q2.collectorC;j[3][2]-=q2.collectorE;j[3][3]-=q2.collectorC;
        residual=0;for(double v:f){if(!std::isfinite(v))return false;residual=std::max(residual,std::abs(v));}
        if(residual<=1e-10)return true;
        if(loops==budget)return false;
        // Fixed4x4 elimination with partial pivoting; stack-only and bounded.
        for(unsigned a=0;a<4;++a){unsigned pivot=a;for(unsigned b=a+1;b<4;++b)if(std::abs(j[b][a])>std::abs(j[pivot][a]))pivot=b;
            if(!std::isfinite(j[pivot][a])||std::abs(j[pivot][a])<1e-16)return false;
            if(pivot!=a){std::swap(j[a],j[pivot]);std::swap(f[a],f[pivot]);}
            for(unsigned b=a+1;b<4;++b){const double scale=j[b][a]/j[a][a];
                for(unsigned k=a+1;k<4;++k)j[b][k]-=scale*j[a][k];
                f[b]-=scale*f[a];}}
        Vector d{};for(int a=3;a>=0;--a){double y=f[a];for(unsigned b=a+1;b<4;++b)y-=j[a][b]*d[b];d[a]=y/j[a][a];}
        double scale=1;
        // Limit growth into exponential forward conduction. Reverse junctions
        // may move by volts; symmetrically limiting them invents solver slew.
        for(auto v:{std::array<double,2>{x[0],d[0]},std::array<double,2>{x[0]-x[1],d[0]-d[1]},
                    std::array<double,2>{x[1]-x[2],d[1]-d[2]},std::array<double,2>{x[1]-x[3],d[1]-d[3]}})
            scale=std::max(scale,-v[1]/std::max(.05,.05-v[0]));
        for(double v:d){if(!std::isfinite(v))return false;scale=std::max(scale,std::abs(v)/2);}
        for(unsigned a=0;a<4;++a)x[a]-=d[a]/scale;
    }
    return false;
}
void Germanium::resetCircuit() noexcept {
    previous_=bias_;zIn_=-bias_[0];zFuzz_=controls_.fuzz*bias_[2];zOut_=-biasNodes_.tap;
    zBc1_=bias_[0]-bias_[1];zBc2_=bias_[1]-bias_[3];nodes_=biasNodes_;nodes_.iterations=0;
}
void Germanium::reset() noexcept {
    resetCircuit();up_.fill(0);down_.fill(0);upIndex_=downIndex_=0;failed_=calls_=evaluations_=contained_=0;maximum_=0;
}
double Germanium::circuitSample(double input) noexcept {
    const double source=-input,bc=2*rate_*100e-12;
    const double tapHistory=tapSupply_+tapHistory_*zOut_;
    const Vector rhs{gin_*(source-zIn_)+bc*zBc1_,circuit_.supplyVolts/circuit_.collector1Ohms-bc*zBc1_+bc*zBc2_,
                     -emitHistory_*zFuzz_,tapHistory/circuit_.collector2Ohms-bc*zBc2_};
    Vector x=previous_;double residual=0;unsigned loops=0;
    const bool ok=solve(x,rhs,passive_,24,residual,loops);
    ++calls_;evaluations_+=loops+1;maximum_=std::max(maximum_,loops+1);
    if(!ok){++failed_;resetCircuit();nodes_.converged=false;nodes_.residualAmps=residual;return 0;}
    const double inputCurrent=gin_*(source-x[0]-zIn_);
    const double tap=tapA_*x[3]+tapHistory,outCurrent=gcapOut_*(tap-zOut_),jack=outCurrent*bottom_;
    const double wiper=wiperA_*x[2]+wiperZ_*zFuzz_;
    zIn_=tidy(2*(source-circuit_.sourceOhms*inputCurrent-x[0])-zIn_);
    zFuzz_=tidy(2*wiper-zFuzz_);zOut_=tidy(2*(tap-volumeR_*outCurrent)-zOut_);
    zBc1_=tidy(2*(x[0]-x[1])-zBc1_);zBc2_=tidy(2*(x[1]-x[3])-zBc2_);
    previous_=x;nodes_={-x[0],-x[1],-x[2],-x[3],-tap,-jack,-inputCurrent,residual,loops+1,true};
    return -jack;
}
float Germanium::process(float input) noexcept {
    if(!std::isfinite(input))return 0;
    const float x=static_cast<float>(tidy(std::clamp(double(input),-1.,1.)));double result=0;
    if(mode_==GermaniumRate::Reference1x)result=circuitSample(x);
    else{
        upIndex_=upIndex_==0?up_.size()-1:upIndex_-1;up_[upIndex_]=x;
        for(std::size_t phase=0;phase<4;++phase){float interpolated=0;auto index=upIndex_;
            for(std::size_t tap=phase;tap<kTaps;tap+=4){interpolated+=fir_[tap]*up_[index];if(++index==up_.size())index=0;}
            downIndex_=downIndex_==0?kTaps-1:downIndex_-1;
            down_[downIndex_]=static_cast<float>(tidy(circuitSample(interpolated*4)));
            if(phase==0){index=downIndex_;for(std::size_t tap=0;tap<kTaps;++tap){result+=fir_[tap]*down_[index];if(++index==kTaps)index=0;}}
        }
    }
    if(!std::isfinite(result)){resetCircuit();++failed_;return 0;}
    if(std::abs(result)>1){++contained_;result=std::clamp(result,-1.,1.);}
    return static_cast<float>(tidy(result));
}
void Germanium::processBlock(const float* input,float* output,std::size_t count) noexcept {
    for(std::size_t i=0;i<count;++i)output[i]=process(input[i]);
}
} // namespace parable::dsp
