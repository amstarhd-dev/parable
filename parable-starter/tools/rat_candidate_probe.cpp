// Diagnostic only: deterministic float PCM for the reference and optimized RAT.
#include "parable_rat.h"
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
using parable::dsp::Rat;
constexpr double pi=3.14159265358979323846;
struct Scenario {float distortion,filter,volume,amplitude;unsigned pattern;unsigned diodeFault;};
int main(int argc,char**argv){
 if(argc!=2){std::cerr<<"Usage: rat_candidate_probe OUTPUT.raw\n";return 2;}
 std::ofstream out(argv[1],std::ios::binary);if(!out)return 3;
 constexpr int rate=48000,frames=rate*2;
 const Scenario scenarios[]={
  {.35f,.5f,.7f,.25f,0},{0.f,0.f,1.f,.9f,1},{1.f,1.f,1.f,.95f,2},
  {1.f,0.f,.05f,.5f,3},{.9f,.4f,.85f,.85f,4},{.5f,.5f,.5f,1.f,5,0},
  {.85f,.6f,.7f,.6f,2,1},  // unilateral diode-open fault
  {1.f,.2f,.65f,.85f,4,2}}; // both diodes disconnected
 for(const auto& c:scenarios){
  Rat rat;parable::dsp::RatCircuit circuit;
  if(c.diodeFault) circuit.positiveDiodeConnected=false;
  if(c.diodeFault==2) circuit.negativeDiodeConnected=false;
  if(!rat.configure(rate,{c.distortion,c.filter,c.volume},circuit))return 4;
  for(int n=0;n<frames;++n){
   const double t=double(n)/rate;double signal=0;
   switch(c.pattern){
   case 0:signal=std::exp(-t*2.7)*(.55*std::sin(2*pi*110*t)+.31*std::sin(2*pi*330*t)+.14*std::sin(2*pi*990*t));break;
   case 1:signal=.4*std::sin(2*pi*80*t)+.35*std::sin(2*pi*140*t)+.25*std::sin(2*pi*740*t);break;
   case 2:signal=std::sin(2*pi*3517*t);break;
   case 3:signal=std::sin(2*pi*7971*t);break;
   case 4:signal=n%4800<600?(n%600<300?1.:-1.):0.;break;
   case 5:{uint32_t s=uint32_t(n)*1664525u+1013904223u;s^=s>>13;s*=0x5bd1e995u;signal=double(s)/4294967295.*2.-1.;break;}
   }
   const float v=rat.process(float(c.amplitude*signal));
   if(!std::isfinite(v)){std::cerr<<"Nonfinite output at "<<n<<"\n";return 5;}
   out.write(reinterpret_cast<const char*>(&v),sizeof(v));
  }
 }
 return out?0:6;
}
