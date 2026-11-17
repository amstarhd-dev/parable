#include "parable_hall.h"
#include "activation_guard.h"
#include "kCathedral.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"CHECK %s:%d %s\n",__FILE__,__LINE__,#x);std::abort();}++checks;}while(0)
using parable::dsp::Hall;
int main(){unsigned checks=0;activation_guard::selfTest();
 auto h=std::make_unique<Hall>(),reference=std::make_unique<Hall>();
 std::array<float,64>in{},left{},right{},a{},b{};
 CHECK(sizeof(Hall)<512*1024);CHECK(Hall::stateBytes()<512*1024);
 CHECK(!h->prepare(32000));CHECK(!h->prepare(std::numeric_limits<float>::quiet_NaN()));
 for(float value:{-1.f,1.01f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
  CHECK(!h->prepare(48000,{value,.35f,0}));CHECK(!h->prepare(48000,{1,value,0}));CHECK(!h->prepare(48000,{1,.35f,value}));}
 h->process(in.data(),left.data(),right.data());CHECK(left==in && right==in);
 for(unsigned rate:{44100u,48000u,96000u}){
  // Unmodified upstream complete processor, same seed, wet-only. Actual signal
  // traverses predelay, early and late networks, not a zero/first-sample smoke.
  std::srand(7);auto upstream=std::make_unique<kCathedral>(nullptr);upstream->setSampleRate(rate);upstream->setParameter(0,1);
  CHECK(h->prepare(rate,{1,1,0}));double peakError=0,stereoDiff=0,monoEnergy=0;
  activation_guard::begin();
  for(size_t n=0;n<size_t(rate)*2;n+=64){for(size_t j=0;j<64;++j){double t=double(n+j)/rate;in[j]=float(.1*std::exp(-t)*std::sin(2*3.141592653589793*131*t));}
   float*ins[]={in.data(),in.data()};float*outs[]={a.data(),b.data()};upstream->processReplacing(ins,outs,64);h->process(in.data(),left.data(),right.data());
   for(size_t j=0;j<64;++j){peakError=std::max({peakError,std::abs(double(left[j])-a[j]),std::abs(double(right[j])-b[j])});stereoDiff+=std::pow(double(left[j])-right[j],2);monoEnergy+=std::pow(double(left[j])+right[j],2);}}
  activation_guard::end();CHECK(peakError==0);CHECK(stereoDiff>1e-4);CHECK(monoEnergy>1e-4);CHECK(h->containmentHits()==0);
  // Reset/stopped reprepare deterministic after audible nonzero history.
  h->reset();CHECK(reference->prepare(rate,{1,1,0}));double resetError=0;
  for(size_t n=0;n<8192;n+=64){in.fill(0);if(n==0)in[0]=.3f;h->process(in.data(),left.data(),right.data());reference->process(in.data(),a.data(),b.data());for(size_t j=0;j<64;++j)resetError=std::max({resetError,std::abs(double(left[j])-a[j]),std::abs(double(right[j])-b[j])});}CHECK(resetError==0);
  // Invalid controls/blocks preserve history. Includes the complete block after
  // one bad value, rather than allowing a partially advanced tank.
  CHECK(!h->prepare(rate,{1,.35f,-.01f}));in.fill(0);in[63]=std::numeric_limits<float>::quiet_NaN();h->process(in.data(),left.data(),right.data());CHECK(std::all_of(left.begin(),left.end(),[](float x){return x==0;}) && std::all_of(right.begin(),right.end(),[](float x){return x==0;}));
  in.fill(0);h->process(in.data(),left.data(),right.data());reference->process(in.data(),a.data(),b.data());CHECK(left==a && right==b);
  for(float focus:{0.f,.98f,1.f})for(float decay:{0.f,1.f}){
   CHECK(h->prepare(rate,{decay,1,focus}));bool finite=true;double late=0,early=0;
   activation_guard::begin();
   for(size_t n=0;n<size_t(rate)*20;n+=64){for(size_t j=0;j<64;++j){double t=double(n+j)/rate;in[j]=t<1?float(.25*std::sin(2*3.141592653589793*173*t)+.12*std::sin(2*3.141592653589793*263*t)):0;}
    h->process(in.data(),left.data(),right.data());for(size_t j=0;j<64;++j){finite=finite&&std::isfinite(left[j])&&std::isfinite(right[j])&&std::abs(left[j])<=.98f&&std::abs(right[j])<=.98f;if(n>=rate&&n<2*rate)early+=double(left[j])*left[j]+double(right[j])*right[j];if(n>=18*rate)late+=double(left[j])*left[j]+double(right[j])*right[j];}}
   activation_guard::end();CHECK(finite);CHECK(late<early*1e-5);CHECK(h->containmentHits()==0);
   in.fill(std::numeric_limits<float>::max());for(int n=0;n<128;++n)h->process(in.data(),left.data(),right.data());CHECK(std::all_of(left.begin(),left.end(),[](float x){return std::isfinite(x)&&std::abs(x)<=.98f;}));
  }
  // Upstream intentionally injects denormal noise/dither: numerical noise floor,
  // not an invented promise of exact silence.
  CHECK(h->prepare(rate,{1,1,.98f}));in.fill(0);double noise=0;
  for(size_t n=0;n<rate;n+=64){h->process(in.data(),left.data(),right.data());for(float x:left)noise=std::max(noise,std::abs(double(x)));}CHECK(noise<1e-6);
 }
 CHECK(activation_guard::clean());std::printf("%u focused checks; exact upstream agreement at3rates; finite decay/reset/callback guards passed\n",checks);
}
