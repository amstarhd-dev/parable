#pragma once
// Narrow MIT ACME.jl bjt adaptation,3832ccafb952ee2ca0a27286c091d613062c23fd.
// Copyright2015,2016,2017 Martin Holters; notice third_party/germanium/ACME-LICENSE.md.
// Measured AC128 facts: Holmes/Holters/vanWalstijn DAFx2017 Table1, equations1,4-7;
// MIT transcription Greg Whitehead2020 (WHITEHEAD-LICENSE.md). Not a universal BJT.
#include "parable_germanium.h"
#include <cmath>
namespace parable::dsp::germanium_detail {
struct Current {
    double collector=0,base=0,collectorE=0,collectorC=0,baseE=0,baseC=0;
    bool valid=false;
};
inline Current device(double ve,double vc,const GermaniumDevice& p) noexcept {
    // Domain containment of solver candidates, not a physical clipping curve.
    if(!std::isfinite(ve)||!std::isfinite(vc)||ve< -12||vc< -12||ve>.8||vc>.8)return {};
    constexpr double vt=.0255,nf=1.133,nr=1.140,ne=1.796,nc=1.364;
    const double ef=std::expm1(ve/(nf*vt)),er=std::expm1(vc/(nr*vt));
    const double el=std::expm1(ve/(ne*vt)),cl=std::expm1(vc/(nc*vt));
    // Paper Is is the transport current, not ACME's alpha*terminal Is convention.
    const double f=p.saturationAmps*ef,r=p.saturationAmps*er;
    const double df=p.saturationAmps*(ef+1)/(nf*vt),dr=p.saturationAmps*(er+1)/(nr*vt);
    const double early=1-ve/88.28-vc/19.68;
    const double root=std::sqrt(1+4*(f/.463+r/.2415)),den=1+root;
    if(!std::isfinite(root)||early<=0)return {};
    const double factor=2*early/den;
    const double factorE=-2/(88.28*den)-4*early*df/(.463*root*den*den);
    const double factorC=-2/(19.68*den)-4*early*dr/(.2415*root*den*den);
    const double be=f/p.forwardGain+p.beLeakageAmps*el;
    const double bc=r/p.reverseGain+p.bcLeakageAmps*cl;
    // Different leakage ideality factors must appear in their own derivatives.
    const double beE=df/p.forwardGain+p.beLeakageAmps*(el+1)/(ne*vt);
    const double bcC=dr/p.reverseGain+p.bcLeakageAmps*(cl+1)/(nc*vt);
    return {factor*(f-r)-bc,be+bc,factorE*(f-r)+factor*df,
            factorC*(f-r)-factor*dr-bcC,beE,bcC,true};
}
} // namespace parable::dsp::germanium_detail
