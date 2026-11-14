#pragma once
// Owned, analytic controlled driver excitations. Model volts, not recorded music.
#include <algorithm>
#include <cmath>
#include <string>
namespace tapelab {
constexpr double pi=3.1415926535897932384626433832795;
inline double stimulus(const std::string& kind,double t,double a=.2) {
    if(t<0)return 0;
    if(kind=="dc")return a;
    if(kind=="tone")return a*std::sin(2*pi*1000*t);
    if(kind=="hf")return a*std::sin(2*pi*10000*t);
    if(kind=="two")return a*.5*(std::sin(2*pi*700*t)+std::sin(2*pi*1100*t));
    if(kind=="chord")return a/3*(std::sin(2*pi*196*t)+std::sin(2*pi*246.94165*t)+std::sin(2*pi*293.66477*t));
    if(kind=="sweep"){constexpr double start=80,end=18000,duration=1;const double k=std::log(end/start)/duration;return t>duration?0:a*std::sin(2*pi*start*std::expm1(k*t)/k);}
    if(kind=="attacks"||kind=="sustain"){
        const double age=kind=="attacks"?std::fmod(t,.22):t;
        return a*(1-std::exp(-age*3000))*std::exp(-age*(kind=="attacks"?12:1.5))*(std::sin(2*pi*196*t)+.25*std::sin(2*pi*392*t))/1.25;
    }
    return 0;
}
} // namespace tapelab
