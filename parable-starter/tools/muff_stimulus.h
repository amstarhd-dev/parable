#pragma once
// Owned controlled science stimulus, not a guitar recording. Shared timing only;
// independent ngspice has its own literal continuous source expression.
#include <cmath>
#include <initializer_list>
namespace mufflab {
constexpr double pi=3.14159265358979323846;
inline float history(double t,bool attack) {
    double x=t>.08?.02*std::sin(2*pi*223*(t-.08)):0;
    if(attack)for(double start:{.02,.22})if(t>start&&t<start+.05) {
        const double age=t-start,s=std::sin(pi*age/.05);
        x+=.7*std::sin(2*pi*110*age)*s*s;
    }
    return float(x);
}
inline float note(double t) {
    const double age=t-.04;if(age<=0)return 0;
    return float(.35*(1-std::exp(-age*800))*std::exp(-age*2.5)*(std::sin(2*pi*110*age)+.15*std::sin(2*pi*330*age)));
}
} // namespace mufflab
