#pragma once
// MIT ACME.jl3832ccafb952ee2ca0a27286c091d613062c23fd transport-current
// Ebers-Moll foundation; specialization to one reciprocal complementary cell.
// Copyright2015/2016/2017 Martin Holters; full notice:
// third_party/wavefolder/ACME-LICENSE.md. Stopped control/science only.
#include <algorithm>
#include <array>
#include <cmath>
namespace parable::dsp::wavefolder_detail {
constexpr double vt=1.380649e-23/1.602176634e-19*300,Is=1e-17,af=100./101,ar=.5;
using Vec=std::array<double,3>;
struct System {Vec residual{},partial{};double a=0,b=0,c=0,d=0,e=0,f=0,g=0;std::array<double,4> junction{};bool valid=false;};
inline System evaluate(const Vec& v,double input,double rp,double rl=10000,double rail=15) noexcept {
 System s;const double j[]={v[0]-input,v[2]-input,input-v[1],input-v[2]};double current[4],conductance[4];
 for(unsigned k=0;k<4;++k){if(!std::isfinite(j[k])||j[k]<-12||j[k]>1.2)return s;const double e=std::expm1(j[k]/vt);current[k]=Is*e;conductance[k]=Is*(e+1)/vt;s.junction[k]=current[k];}
 const auto fp=current[0],rpC=current[1],fn=current[2],rnC=current[3],gp=conductance[0],gr=conductance[1],gn=conductance[2],gs=conductance[3];
 s.residual={(v[0]-rail)/rp+fp/af-rpC,(v[1]+rail)/15000-fn/af+rnC,v[2]/rl-fp+rpC/ar+fn-rnC/ar};
 s.a=1/rp+gp/af;s.b=1./15000+gn/af;s.c=-gr;s.d=-gs;s.e=-gp;s.f=-gn;s.g=1/rl+(gr+gs)/ar;
 s.partial={-gp/af+gr,-gn/af+gs,gp-gr/ar+gn-gs/ar};s.valid=true;return s;
}
inline double norm(const Vec& v) noexcept {return std::max({std::abs(v[0]),std::abs(v[1]),std::abs(v[2])});}
inline Vec inverse(const System& s,const Vec& rhs) noexcept {const double co=(rhs[2]-s.e*rhs[0]/s.a-s.f*rhs[1]/s.b)/(s.g-s.e*s.c/s.a-s.f*s.d/s.b);return {(rhs[0]-s.c*co)/s.a,(rhs[1]-s.d*co)/s.b,co};}
struct Point {Vec nodes{};double value=0,slope=0;std::array<double,4> junction{};unsigned iterations=0;bool valid=false;};
inline Point solve(double input,double rp=15000,double rl=10000,double rail=15,unsigned maximum=64,const Vec* previous=nullptr) noexcept {
 Point p;p.nodes=previous?*previous:Vec{input+.83,input-.83,input};
 for(unsigned n=0;n<maximum;++n){const auto s=evaluate(p.nodes,input,rp,rl,rail);if(!s.valid)return p;
  if(norm(s.residual)<5e-15){auto rhs=s.partial;for(auto& x:rhs)x=-x;const auto d=inverse(s,rhs);p.value=-p.nodes[2];p.slope=-d[2];p.junction=s.junction;p.iterations=n;p.valid=true;return p;}
  Vec rhs=s.residual;for(auto& x:rhs)x=-x;auto step=inverse(s,rhs);const double scale=std::min(1.,.25/norm(step));for(auto& v:step)v*=scale;
  bool accepted=false;double damping=1;for(unsigned backtrack=0;backtrack<20;++backtrack){auto trial=p.nodes;for(unsigned k=0;k<3;++k)trial[k]+=damping*step[k];const auto next=evaluate(trial,input,rp,rl,rail);
   if(next.valid&&norm(next.residual)<norm(s.residual)){p.nodes=trial;accepted=true;break;}damping*=.5;}
  if(!accepted)return p;
 }
 return p;
}
// Host/control science: actual emitter equilibrium at an interpolated collector
// value. Its residual exposes the LUT's electrical error, rather than reporting
// the exact solver's KCL as if it were the interpolation's KCL.
inline System observe(double input,double outputReadout,Vec& nodes,double rp=15000) noexcept {
 nodes[2]=-outputReadout;
 for(unsigned branch=0;branch<2;++branch){const double reverse=Is*std::expm1((branch?input-nodes[2]:nodes[2]-input)/vt),r=branch?15000:rp,bias=branch?15+input:15-input;double lo=0,hi=1.2;
  for(unsigned n=0;n<52;++n){const double m=(lo+hi)/2,res=(m-bias)/r+Is*std::expm1(m/vt)/af-reverse;if(res>0)hi=m;else lo=m;}
  nodes[branch]=branch?input-(lo+hi)/2:input+(lo+hi)/2;
 }
 return evaluate(nodes,input,rp);
}
}
