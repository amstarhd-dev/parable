#include "parable_rat.h"
#include "test_check.h"
#include "rat_measure.h"
#include <cstring>
#include <cstdint>
#include <limits>
#include <new>
using namespace parable::dsp;
static bool watching=false;static std::size_t allocations=0;
void* operator new(std::size_t n){if(watching)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
static void bounded(float v){CHECK(std::isfinite(v));CHECK(std::abs(v)<=1);}
static void controlsAndState(){
    Rat a,b;CHECK(a.configure(48000));CHECK(b.configure(48000));
    CHECK(a.process(.1f)==b.process(.1f));
    for(float value:{NAN,INFINITY,-INFINITY,-1.f,1.1f})for(int field=0;field<3;++field){
        RatControls p;switch(field){case 0:p.distortion=value;break;case 1:p.filter=value;break;case 2:p.volume=value;break;}
        CHECK(!a.configure(48000,p));CHECK(a.process(.2f)==b.process(.2f));}
    for(float rate:{NAN,INFINITY,-1.f,0.f,31999.f,96001.f})CHECK(!a.configure(rate));
    for(int field=0;field<9;++field){RatCircuit c;
        switch(field){case 0:c.feedbackBassOhms=0;break;case 1:c.feedbackCompFarads=NAN;break;
        case 2:c.diodeNegativeIsAmps=INFINITY;break;case 3:c.couplingFarads=0;break;
        case 4:c.supplyVolts=1;break;case 5:c.opampGbwHz=-1;break;case 6:c.diodeIdeality=0;break;
        case 7:c.railMarginVolts=20;break;case 8:c.volumePotOhms=NAN;break;}
        CHECK(!a.configure(48000,{},c));CHECK(a.process(-.1f)==b.process(-.1f));}
    CHECK(!a.configure(48000,{}, {},static_cast<RatRate>(99)));
}
static void blocks(){
    std::array<float,1027> source{},expected{},actual{};
    for(std::size_t i=0;i<source.size();++i)source[i]=static_cast<float>(.2*std::sin(i*.147));
    for(auto mode:{RatRate::Reference1x,RatRate::Oversampled4x}){
        Rat a,b;CHECK(a.configure(44100,{}, {},mode));CHECK(b.configure(44100,{}, {},mode));
        for(std::size_t i=0;i<source.size();++i)expected[i]=a.process(source[i]);
        actual=source;b.processBlock(actual.data(),actual.data(),0);b.processBlock(actual.data(),actual.data(),17);
        b.processBlock(actual.data()+17,actual.data()+17,actual.size()-17);CHECK(actual==expected);
        b.reset();b.processBlock(source.data(),actual.data(),source.size());CHECK(actual==expected);
        for(float bad:{NAN,INFINITY,-INFINITY}){CHECK(a.process(bad)==0);CHECK(a.process(.1f)==b.process(.1f));}
    }
}
static void stress(){
    uint32_t random=0x52415437;const float max=std::numeric_limits<float>::max();
    for(float rate:{32000.f,48000.f,96000.f})for(const auto* name:{"healthy","diode_open","ruetz","compensation"}){
        Rat rat;auto c=ratlab::variation(name);CHECK(rat.configure(rate,{1,1,1},c));
        for(int i=0;i<256;++i)CHECK(rat.process(0)==0);
        CHECK(rat.process(std::numeric_limits<float>::denorm_min())==0);
        watching=true;
        for(int i=0;i<8192;++i){random=random*1664525u+1013904223u;float x;std::memcpy(&x,&random,4);
            bounded(rat.process(x));bounded(rat.process(i%2?max:-max));}
        watching=false;CHECK(allocations==0);
        for(int i=0;i<static_cast<int>(rate/4);++i)bounded(rat.process(0));
        rat.reset();for(int i=0;i<256;++i)CHECK(rat.process(0)==0);
        CHECK(rat.configure(rate,{.5f,.5f,0},c));for(int i=0;i<256;++i)CHECK(rat.process(.3f)==0);
    }
    // Long physical RC recovery (10uF output coupling), tested without resampler overhead.
    Rat r;CHECK(r.configure(48000,{.5f,.5f,.7f},ratlab::variation("diode_open"),RatRate::Reference1x));
    for(int i=0;i<48000;++i)bounded(r.process(.2f*static_cast<float>(std::sin(i*.13))));
    for(int i=0;i<48000*25;++i)bounded(r.process(.5f));
    CHECK(std::abs(r.process(.5f))<1e-8);
    for(int i=0;i<48000*25;++i)bounded(r.process(0));
    CHECK(std::abs(r.process(0))<1e-8);
}
static void slewBound(){
    for(double slew:{.03,.3}){Rat r;RatCircuit c;c.opampSlewVoltsPerUs=slew;
        CHECK(r.configure(48000,{.9f,0,.7f},c,RatRate::Reference1x));float previous=0;
        for(int i=0;i<1024;++i){bounded(r.process(i%64<32?1.f:-1.f));const float now=r.lastNodes().opamp;
            CHECK(std::abs(now-previous)<=slew*1e6/48000+1e-6);previous=now;}}
}
static void stoppedFaultTransitions(){
    Rat reused;
    for(const auto* name:{"diode_open","ruetz","compensation","healthy"}){
        for(int i=0;i<1024;++i)bounded(reused.process(.3f*static_cast<float>(std::sin(i*.17))));
        const auto circuit=ratlab::variation(name);Rat fresh;
        CHECK(reused.configure(48000,{.35f,.5f,.7f},circuit));
        CHECK(fresh.configure(48000,{.35f,.5f,.7f},circuit));
        for(int i=0;i<1024;++i){const float x=.2f*static_cast<float>(std::sin(i*.137));
            CHECK(reused.process(x)==fresh.process(x));}
    }
}
int main(){controlsAndState();blocks();stress();slewBound();stoppedFaultTransitions();const auto e=ratlab::spiceReferences();
    std::cout<<"RAT checks "<<testChecks<<", allocation count "<<allocations<<", SPICE errors gain dB "<<e.maxGainDb
        <<" phase deg "<<e.maxPhaseDegrees<<" transient RMS/peak V "<<e.transientRms<<'/'<<e.transientPeak<<'\n';}
