#pragma once
// MIT ACME.jl diode/bjt/finite-pole opamp foundation,3832ccafb952ee2ca0a27286c091d613062c23fd.
// Copyright2015/2016/2017 Martin Holters. Full notice: third_party/ts808/ACME-LICENSE.md.
// Nominal silicon, not a calibrated2SC1815/MA150 or vintage4558 sample.
#include "parable_ts808.h"
#include <algorithm>
#include <cmath>
namespace parable::dsp::ts808_detail {
constexpr double vt=(1.380649e-23/1.602176634e-19)*300, bf=350, br=4;
constexpr double is=2.52e-9, nv=1.752*vt, wp=188.4955592153876, gain=1e5, slew=1e6, railG=1e9;
struct Device { Ts808::Currents i{},g{}; std::array<double,2> diodes{}; bool valid=false; };
inline Device currents(const Ts808::Unknowns& v,bool openD2) noexcept {
    Device out;
    for(std::size_t k=0;k<4;++k) {
        if(!std::isfinite(v[k])||v[k]<-12||v[k]>.85)return {};
        const double e=std::expm1(v[k]/vt);
        out.i[k]=1e-14*e; out.g[k]=1e-14*(e+1)/vt;
    }
    if(!std::isfinite(v[4])||v[4]<-1.25||v[4]>(openD2?12:1.25))return {};
    // D2 is genuinely absent: neither its current nor derivative is evaluated.
    const double negative=std::expm1(-v[4]/nv);
    out.diodes[0]=is*negative;out.i[4]=-out.diodes[0]; out.g[4]=is*(negative+1)/nv;
    if(!openD2) {
        const double positive=std::expm1(v[4]/nv);
        out.diodes[1]=is*positive;out.i[4]+=out.diodes[1]; out.g[4]+=is*(positive+1)/nv;
    }
    out.valid=true; return out;
}
struct Amplifier { double rate=0,drive=0,errorDerivative=0,stateDerivative=0; bool rail=false,limited=false; };
inline Amplifier amplifier(double error,double u) noexcept {
    const double raw=wp*(gain*error-u),excess=u-std::clamp(u,-3.,3.);
    const bool active=std::abs(raw)<slew;
    return {std::clamp(raw,-slew,slew)-railG*excess,std::clamp(raw,-slew,slew),
            active?wp*gain:0, active?-wp:0,excess!=0,!active};
}
struct Response {double value=0,derivative=0;};
inline Response response(double error,double previous,double step,bool dc) noexcept {
    const double den=dc?wp:1+step*wp,coefficient=dc?wp*gain:step*wp*gain;
    const double base=(dc?0:previous)+coefficient*error,g=dc?railG:step*railG;
    double u=base/den,derivative=coefficient/den;
    const double edge=std::clamp(u,-3.,3.);
    if(u!=edge){u=edge+(base-edge*den)/(den+g);derivative=coefficient/(den+g);}
    const double raw=wp*(gain*error-u);
    if(std::abs(raw)>slew) {
        const double sign=raw>0?1:-1;
        if(dc)u=sign*(3+slew/railG);
        else {
            const double target=previous+step*sign*slew,rail=std::clamp(target,-3.,3.);
            u=rail+(target-rail)/(1+(target==rail?0:g));
        }
        derivative=0;
    }
    return {u,derivative};
}
} // namespace parable::dsp::ts808_detail
