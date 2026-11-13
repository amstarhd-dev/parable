#pragma once
#include <cmath>
namespace ts808lab {
constexpr double pi=3.14159265358979323846;
inline float periodic(double time,double amplitude,double frequency,bool triangle=false) {
    const double phase=std::sin(2*pi*frequency*time);
    return float(amplitude*(triangle?2/pi*std::asin(phase):phase));
}
// Balanced bipolar envelopes: the same input reaches both circuit configurations.
inline float attacks(double time) {
    double v=0;
    for(unsigned k=0;k<3;++k) {
        const double age=time-(.02+.22*k);
        if(age>=0&&age<.18) {
            const double amplitude=k==0?.01:k==1?.08:.4;
            v+=amplitude*(1-std::exp(-age*1200))*std::exp(-age*18)*std::sin(2*pi*220*age);
        }
    }
    return float(v);
}
inline float note(double time) {
    return time<1.5?float(.12*(1-std::exp(-time*1000))*std::exp(-time*1.5)*std::sin(2*pi*196*time)):0;
}
}
