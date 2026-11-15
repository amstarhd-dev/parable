#pragma once
// Owned controlled stimuli, not recordings or generated accompaniment.
#include <cmath>
#include <string>
namespace foldlab {
constexpr double pi=3.1415926535897932384626433832795;
inline double stimulus(const std::string& kind,double t) {
 if(t<0)return 0;
 if(kind=="sweep"){const double k=std::log(18000./80);return t>1?0:.2*std::sin(2*pi*80*std::expm1(k*t)/k);}
 if(kind=="chord")return .2/3*(std::sin(2*pi*196*t)+std::sin(2*pi*246.94165*t)+std::sin(2*pi*293.66477*t));
 if(kind=="two")return .1*(std::sin(2*pi*1397*t)+std::sin(2*pi*1763*t));
 const double age=kind=="attacks"?std::fmod(t,.22):t;
 return .2*(1-std::exp(-age*3000))*std::exp(-age*(kind=="attacks"?12:1.5))*(std::sin(2*pi*196*t)+.25*std::sin(2*pi*392*t))/1.25;
}
} // namespace foldlab
