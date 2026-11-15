#pragma once
// Reciprocal Ebers-Moll exponential/derivative foundation adapted via main Muff
// from MIT ACME.jl3832ccafb952ee2ca0a27286c091d613062c23fd, Martin Holters
// Copyright2015/16/17; complete notice third_party/blender/ACME-LICENSE.md.
// Named1N276 physical facts: Novender SCEPTRE p106/Cordwell transcription.
#include <cmath>
namespace parable::dsp::blender_detail {
constexpr double kOverQ=1.380649e-23/1.602176634e-19;
constexpr double diodeN=1/(32*kOverQ*300),transit=4.0625e-9;
struct Junction {double i=0,g=0,q=0,c=0;bool valid=false;};
inline Junction junction(double voltage,double inverseThermal,double saturation,double tt=0) noexcept {
    if(!std::isfinite(voltage)||voltage< -20||voltage>1)return {};
    const double e=std::expm1(voltage*inverseThermal);
    const double i=saturation*e,g=saturation*(e+1)*inverseThermal;
    return {i,g,tt*i,tt*g,true};
}
} // namespace parable::dsp::blender_detail
