#pragma once
// Same MIT-derived GP equations as germanium_device.h (retained byte for byte).
// Only the exponential value/analytic-derivative provider differs.
#include "germanium_device.h"
#include "germanium_exp.h"
namespace parable::dsp::germanium_detail {
template<class Evaluator>
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
inline Current deviceApprox(double ve,double vc,const GermaniumDevice& p) noexcept {
    if(!std::isfinite(ve)||!std::isfinite(vc)||ve< -12||vc< -12||ve>.8||vc>.8)return {};
    constexpr double vt=.0255,nf=1.133,nr=1.140,ne=1.796,nc=1.364;
    constexpr double sf=1/(nf*vt),sr=1/(nr*vt),se=1/(ne*vt),sc=1/(nc*vt);
    const auto ef=Evaluator::evaluate(ve*sf),er=Evaluator::evaluate(vc*sr);
    const auto el=Evaluator::evaluate(ve*se),cl=Evaluator::evaluate(vc*sc);
    const double f=p.saturationAmps*ef.value,r=p.saturationAmps*er.value;
    const double df=p.saturationAmps*ef.derivative*sf,dr=p.saturationAmps*er.derivative*sr;
    const double early=1-ve/88.28-vc/19.68;
    const double root=std::sqrt(1+4*(f/.463+r/.2415)),den=1+root;
    if(!std::isfinite(root)||early<=0)return {};
    const double factor=2*early/den;
    const double factorE=-2/(88.28*den)-4*early*df/(.463*root*den*den);
    const double factorC=-2/(19.68*den)-4*early*dr/(.2415*root*den*den);
    const double be=f/p.forwardGain+p.beLeakageAmps*el.value;
    const double bc=r/p.reverseGain+p.bcLeakageAmps*cl.value;
    const double beE=df/p.forwardGain+p.beLeakageAmps*el.derivative*se;
    const double bcC=dr/p.reverseGain+p.bcLeakageAmps*cl.derivative*sc;
    return {factor*(f-r)-bc,be+bc,factorE*(f-r)+factor*df,
            factorC*(f-r)-factor*dr-bcC,beE,bcC,true};
}
} // namespace parable::dsp::germanium_detail
