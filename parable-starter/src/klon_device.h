#pragma once
// MIT ACME.jl diode/finite-pole foundation,3832ccafb952ee2ca0a27286c091d613062c23fd.
// Copyright2015/2016/2017 Martin Holters; third_party/klon/ACME-LICENSE.md.
// Nominal reduced Ge/TL072CP, not calibrated vintage parts or TL072H.
#include <algorithm>
#include <cmath>
namespace parable::dsp::klon_detail {
constexpr double vt=(1.380649e-23/1.602176634e-19)*300,nv=1.5*vt,is=600e-9;
constexpr double g0=2*is/nv,wp=94.2477796076938,gain=200000,slew=13e6,railG=1e10;
struct Device {double i=0,g=0;double d2=0,d3=0;bool valid=false;};
inline Device diode(double v) noexcept {
    if(!std::isfinite(v)||std::abs(v)>1.25)return {};
    const double p=std::expm1(v/nv),n=std::expm1(-v/nv);
    return {is*(p-n)-g0*v,is*(p+n+2)/nv-g0,is*n,is*p,true};
}
struct Response {double value=0,slope=0;};
// Exact scalar elimination of each amplifier's own linear feedback loading.
// Other amplifier states and diode current remain coupled in the5x5 solve.
inline Response response(double error,double self,double previous,double step,bool dc,std::size_t amp) noexcept {
    const double pole=wp*(1-gain*self),coefficient=wp*gain;
    const double den=dc?pole:1+step*pole,c=dc?coefficient:step*coefficient;
    const double base=(dc?0:previous)+c*error,g=dc?railG:step*railG;
    const double lo=amp<2?-3:-11.6,hi=amp<2?3:10.2;
    double u=base/den,slope=c/den;
    const double edge=std::clamp(u,lo,hi);
    if(u!=edge){u=edge+(base-edge*den)/(den+g);slope=c/(den+g);}
    const double raw=coefficient*error-pole*u;
    if(std::abs(raw)>slew) {
        const double sign=raw>0?1:-1;
        if(dc)u=(sign>0?hi:lo)+sign*slew/railG;
        else {
            const double target=previous+step*sign*slew,rail=std::clamp(target,lo,hi);
            u=rail+(target-rail)/(1+(target==rail?0:g));
        }
        slope=0;
    }
    return {u,slope};
}
} // namespace parable::dsp::klon_detail
