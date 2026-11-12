#pragma once
// Narrow MIT ACME.jl bjt Ebers-Moll specialization,3832ccafb952ee2ca0a27286c091d613062c23fd.
// Copyright2015/2016/2017 Martin Holters; third_party/muff/ACME-LICENSE.md.
// Transport Is (ACME terminal Is/alpha conversion), not measured Ge constants.
#include "parable_muff.h"
#include <cmath>
namespace parable::dsp::muff_detail {
constexpr double vt=(1.380649e-23/1.602176634e-19)*300,diodeIs=2.52e-9,diodeN=1.752;
constexpr double diodeG0=2*diodeIs/(diodeN*vt),bf=300,br=4;
struct Currents {Muff::Ports i{},g{};bool valid=false;};
inline Currents currents(const Muff::Ports& v) noexcept {
    Currents out;
    for(std::size_t k=0;k<8;++k) {
        if(!std::isfinite(v[k])||v[k]<-12||v[k]>.85)return {};
        const double e=std::expm1(v[k]/vt);
        out.i[k]=1e-14*e;out.g[k]=1e-14*(e+1)/vt;
    }
    for(std::size_t k=8;k<10;++k) {
        if(!std::isfinite(v[k])||std::abs(v[k])>1.25)return {};
        const double e=std::expm1(v[k]/(diodeN*vt)),r=std::expm1(-v[k]/(diodeN*vt));
        // Exact algebraic split; zero-bias conductance is already in passive A.
        out.i[k]=diodeIs*(e-r)-diodeG0*v[k];
        out.g[k]=diodeIs/(diodeN*vt)*(e+r);
    }
    out.valid=true;return out;
}
} // namespace parable::dsp::muff_detail
