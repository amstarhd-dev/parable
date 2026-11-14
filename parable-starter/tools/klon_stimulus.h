#pragma once
// Owned controlled excitation; synthetic, not recorded guitar.
#include <cmath>
namespace klonlab {
constexpr double pi=3.14159265358979323846;
inline float periodic(double t,double amplitude,double hz,bool triangle=false) {
    const double s=std::sin(2*pi*hz*t);
    return float(amplitude*(triangle?2/pi*std::asin(s):s));
}
inline float attacks(double t) {
    for(unsigned k=0;k<3;++k){const double age=t-(.03+.22*k);if(age>=0&&age<.18){const double a=k==0?.015:k==1?.1:.4;return float(a*(1-std::exp(-age*1500))*std::exp(-age*18)*(std::sin(2*pi*196*age)+.25*std::sin(2*pi*392*age)));}}
    return 0;
}
inline float note(double t) {
    if(t<.03||t>=1.5)return 0;
    const double age=t-.03;
    return float(.13*(1-std::exp(-age*2000))*std::exp(-age*2)*std::sin(2*pi*146.832384*age));
}
inline float impulse(double t) {return t>=.03&&t<.0301?.2f:t>=.031&&t<.0311?-.2f:0;}
inline float chord(double t) {
    if(t<.03||t>=2)return 0;
    const double age=t-.03;
    return float(.08*(1-std::exp(-age*1200))*std::exp(-age*2.5)*(std::sin(2*pi*164.813778*age)+.8*std::sin(2*pi*207.652349*age)+.65*std::sin(2*pi*246.941651*age)));
}
} // namespace klonlab
