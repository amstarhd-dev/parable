#pragma once
// Directional relay/hold foundation derived from Isaac Kramer's python-preisach,
// MIT2023,0b27d738319e78160236068192aa7be3058bbbc9. Full notice in
// third_party/tape_saturation/Kramer-LICENSE.txt. Ordered Gaussian population
// condensation, passivity analysis and moving parcels are Parable adaptations.
#include <algorithm>
#include <array>
#include <cmath>
namespace parable::dsp::tape_saturation_detail {
constexpr double pi=3.1415926535897932384626433832795,sigma=12000;
constexpr std::array<double,8> radius={15000,18000,21000,24000,27000,30000,33000,36000};
inline double advance(double p,double h,double r) noexcept {
    if(h-r>p)return h-r;
    if(h+r<p)return h+r;
    return p;
}
inline void prepareCDF(std::array<float,129>& v,std::array<float,129>& d) noexcept {
    constexpr double h=1./16;
    for(std::size_t i=0;i<v.size();++i){const double x=i*h;v[i]=float(std::erf(x/std::sqrt(2.)));d[i]=float(std::sqrt(2/pi)*std::exp(-x*x/2));}
    // Positive, shape-preserving Hermite slopes, including float-rounded tails.
    // Reducing a shared slope cannot invalidate an earlier interval's bound.
    for(std::size_t i=0;i+1<v.size();++i){const double delta=(v[i+1]-v[i])/h,sum=d[i]+d[i+1];
        if(delta<=0){d[i]=d[i+1]=0;}
        else if(sum>3*delta){const double scale=3*delta/sum;d[i]=float(d[i]*scale);d[i+1]=float(d[i+1]*scale);}}
}
inline double gaussian(double x,const std::array<float,129>& v,const std::array<float,129>& d) noexcept {
    const double a=std::abs(x);if(a>=8)return x<0?-1:1;
    const double u=a*16;const auto i=std::size_t(u);const double t=u-i,h=1./16;
    const double c=h*d[i],b=3*(v[i+1]-v[i])-h*(2*d[i]+d[i+1]);
    const double cubic=2*(v[i]-v[i+1])+h*(d[i]+d[i+1]);
    const double y=((cubic*t+b)*t+c)*t+v[i];return x<0?-y:y;
}
} // namespace parable::dsp::tape_saturation_detail
