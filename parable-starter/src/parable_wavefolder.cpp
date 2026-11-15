#include "parable_wavefolder.h"
#include "wavefolder_cell.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace parable::dsp {
namespace {
constexpr double pi=3.1415926535897932384626433832795,spacing=6./Wavefolder::kIntervals;
void count(uint32_t& v) noexcept {if(v!=std::numeric_limits<uint32_t>::max())++v;}
bool validDrive(float d) noexcept {return std::isfinite(d)&&d>=0&&d<=6;}
}
bool Wavefolder::prepare(float rate,WavefolderControls c,unsigned factor,bool antialias) noexcept {
    if(!std::isfinite(rate)||rate<44100||rate>96000||!validDrive(c.drive)||(factor!=1&&factor!=2&&factor!=4))return false;
    Wavefolder next;next.rate_=rate;next.controls_=c;next.factor_=factor;next.taps_=48*factor+1;next.antialias_=antialias;
    wavefolder_detail::Vec previous{};bool havePrevious=false;
    for(std::size_t n=0;n<kKnots;++n){const double x=-3+n*spacing;const auto p=wavefolder_detail::solve(x,next.upperFeedOhms(),10000,15,64,havePrevious?&previous:nullptr);
        if(!p.valid)return false;
        next.values_[n]=float(p.value);next.slopes_[n]=float(p.slope);previous=p.nodes;havePrevious=true;
        if(n){const double a=next.values_[n-1],b=next.values_[n],da=next.slopes_[n-1],db=next.slopes_[n];
            next.integrals_[n]=next.integrals_[n-1]+spacing*((a+b)/2+spacing*(da-db)/12);}
    }
    if(factor>1){double sum=0;for(unsigned n=0;n<=(next.taps_-1)/2;++n){const double x=double(n)-(next.taps_-1)/2.,cutoff=.4/factor;
        next.fir_[n]=float((x==0?2*cutoff:std::sin(2*pi*cutoff*x)/(pi*x))*(.42-.5*std::cos(2*pi*n/(next.taps_-1))+.08*std::cos(4*pi*n/(next.taps_-1))));sum+=next.fir_[n]*(n==(next.taps_-1)/2?1:2);}
        for(auto& a:next.fir_)a=float(a/sum);}
    // Common isolated100nF/100k output coupling, prewarped bilinear RC.
    const double g=std::tan(pi*(1/(2*pi*.01))/rate);next.hpG_=g/(1+g);
    next.prepared_=true;next.reset();*this=next;return true;
}
Wavefolder::CellValue Wavefolder::cellCurve(double x) const noexcept {
    if(!prepared_||!std::isfinite(x))return {};
    x=std::clamp(x,-3.,3.);const double q=(x+3)/spacing;const auto i=std::min(std::size_t(q),kIntervals-1);const double t=q-i;
    const double a=values_[i],d=spacing*slopes_[i],b=3*(double(values_[i+1])-a)-2*d-spacing*slopes_[i+1],c=2*(a-values_[i+1])+d+spacing*slopes_[i+1];
    return {a+t*(d+t*(b+t*c)),integrals_[i]+spacing*t*(a+t*(d/2+t*(b/3+t*c/4)))};
}
void Wavefolder::reset() noexcept {
    upIndex_=downIndex_=0;up_.fill(0);const auto p=cellCurve(0);dc_=p.volts;down_.fill(float(dc_));lastX_=0;lastF_=p.primitive;hpState_=0;
    // FIR and coupling start at the actual circuit DC point. This is the same
    // linear circuit with a precharged capacitor; the raw cell bias is retained.
    if(factor_>1){for(unsigned n=0;n<taps_;++n)hpState_+=double(coefficient(n))*float(dc_);}else hpState_=dc_;
    probe_={};invalid_=inputClamps_=tableClamps_=outputClamps_=failures_=0;running_=prepared_;
}
bool Wavefolder::setDrive(float drive) noexcept {if(!validDrive(drive))return false;controls_.drive=drive;return true;}
double Wavefolder::sample(double x) noexcept {
    if(std::abs(x)>3){count(tableClamps_);x=std::clamp(x,-3.,3.);}
    const auto p=cellCurve(x);const double dx=x-lastX_;
    const double result=antialias_?(std::abs(dx)<1e-5?cellCurve(.5*(x+lastX_)).volts:(p.primitive-lastF_)/dx):p.volts;
    lastX_=x;lastF_=p.primitive;probe_.cellInputVolts=x;probe_.instantCellVolts=p.volts;probe_.averagedCellVolts=result;return result;
}
float Wavefolder::process(float input) noexcept {
    if(!running_)return 0;
    if(!std::isfinite(input)){count(invalid_);return 0;}
    if(std::abs(input)>.25f){count(inputClamps_);input=std::clamp(input,-.25f,.25f);}
    double result=0;
    if(factor_==1)result=sample(double(input)*controls_.drive);
    else {upIndex_=upIndex_?upIndex_-1:48;up_[upIndex_]=input;
        for(unsigned phase=0;phase<factor_;++phase){float interpolated=0;unsigned i=upIndex_;
            for(unsigned tap=phase;tap<taps_;tap+=factor_){interpolated+=coefficient(tap)*up_[i];if(++i==up_.size())i=0;}
            const auto value=sample(double(interpolated*factor_)*controls_.drive);downIndex_=downIndex_?downIndex_-1:taps_-1;down_[downIndex_]=float(value);
            if(phase==0){i=downIndex_;for(unsigned tap=0;tap<taps_;++tap){result+=double(coefficient(tap))*down_[i];if(++i==taps_)i=0;}}
        }
    }
    probe_.filteredVolts=result;const double v=(result-hpState_)*hpG_,low=v+hpState_;hpState_=low+v;probe_.outputVolts=result-low;result=probe_.outputVolts/8;
    if(!std::isfinite(result)){count(failures_);return 0;}
    if(std::abs(result)>1){count(outputClamps_);result=std::clamp(result,-1.,1.);}return float(result);
}
} // namespace parable::dsp
