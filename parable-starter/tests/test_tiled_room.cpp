#include <algorithm>
#include <cassert>
#include <cstring>
#include "StaticPlate.hpp"
#include "parable_tiled_room.h"
#include "activation_guard.h"
#include "test_check.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>
using namespace parable::soundforge;
int main(){
 activation_guard::selfTest();double worst=0;
 for(float rate:{44100.f,48000.f,96000.f}){
  CHECK(TiledRoom::requiredSamples(rate)<=TiledRoom::kMaxSamples);std::vector<float> mem(TiledRoom::requiredSamples(rate)),other(mem.size());TiledRoom e,twin;
  for(float decay:{.25f,.62f,1.2f}){
   PlateReverb<float,int> ref;ref.setSampleRate(rate);ref.setMix(1);ref.setPredelay(.004f);ref.setLowpass(10000);ref.setDamping(6500);ref.setSize(.3f);
   CHECK(e.prepare(rate,mem.data(),mem.size(),{decay,1,false}));ref.setDecay(e.loopGain());double error=0,energy=0;unsigned random=123;
   activation_guard::begin();
   for(unsigned i=0;i<unsigned(rate);++i){random=random*1664525+1013904223;const float x=i<rate/4?float(int(random>>16)-32768)/327680:0;float l,r;ref.process(x*.5f,x*.5f,&l,&r);auto y=e.process(x);error=std::max({error,double(std::abs(.5f*l-y.wetLeft)),double(std::abs(.5f*r-y.wetRight))});energy+=double(y.mono)*y.mono;}
   activation_guard::end();CHECK(error<2e-6);CHECK(energy>0&&std::isfinite(energy));worst=std::max(worst,error);
  }
  for(bool fault:{false,true}){
   TiledSettings c{.62f,.35f,fault};CHECK(e.prepare(rate,mem.data(),mem.size(),c));CHECK(twin.prepare(rate,other.data(),other.size(),c));
   const auto first=e.process(.2f);e.reset();const auto again=e.process(.2f);CHECK(first.mono==again.mono);CHECK(twin.process(.2f).mono==again.mono);
   for(float invalid:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),2.f})CHECK(e.process(invalid).mono==0);
   auto bad=c;bad.decaySeconds=0;CHECK(!e.prepare(rate,mem.data(),mem.size(),bad));CHECK(!e.prepare(rate,nullptr,mem.size(),c));CHECK(!e.prepare(rate,mem.data(),mem.size()-1,c));
   bool identical=true,finite=true;double last=0;activation_guard::begin();
   for(unsigned n=0;n<unsigned(rate*5);++n){const float x=n<rate?float(.2*std::sin(n*.1)):0;const auto a=e.process(x),b=twin.process(x);identical&=a.mono==b.mono;finite&=std::isfinite(a.left)&&std::isfinite(a.right);if(n>rate*4)last+=double(a.mono)*a.mono;}
   activation_guard::end();CHECK(identical&&finite);CHECK(last<.001);CHECK(e.diffuserGain()==(fault?.94f:.5f));
   // Reset after an audible history must equal a newly constructed processor.
   e.reset();TiledRoom fresh;std::vector<float> freshMemory(mem.size());CHECK(fresh.prepare(rate,freshMemory.data(),freshMemory.size(),c));bool resetSame=true,dcFinite=true;double maxDC=0;
   activation_guard::begin();
   for(unsigned n=0;n<unsigned(rate/2);++n)resetSame&=e.process(n==0?.2f:0).mono==fresh.process(n==0?.2f:0).mono;
   for(unsigned n=0;n<unsigned(2*rate);++n){const auto y=e.process(n<rate?1.f:-1.f);dcFinite&=std::isfinite(y.left)&&std::isfinite(y.right);maxDC=std::max(maxDC,double(std::abs(y.mono)));}
   activation_guard::end();CHECK(resetSame);CHECK(dcFinite&&std::isfinite(maxDC)&&maxDC<128); // numerical bound, not unit-output headroom

  }
  CHECK(e.prepare(rate,mem.data(),mem.size(),{.62f,0,false}));CHECK(e.process(.125f).mono==.125f);
 }
 TiledRoom e;CHECK(e.process(.1f).mono==0);
 for(float badRate:{0.f,32000.f,44101.f,std::numeric_limits<float>::quiet_NaN()}){CHECK(TiledRoom::requiredSamples(badRate)==0);}
 float storage[4]{};CHECK(!e.prepare(48000,storage,4));
 CHECK(activation_guard::clean());std::cout<<testChecks<<" focused checks; upstream healthy maximum error="<<worst<<";3rates, controls/reset/history, healthy/fault decay, callback guards\n";
}
