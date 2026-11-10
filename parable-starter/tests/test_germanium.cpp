#include "germanium_measure.h"
#include "test_check.h"
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
using namespace parable::dsp;
static bool watching=false;static std::size_t allocations=0;
void* operator new(std::size_t n){if(watching)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void bounded(float x){CHECK(std::isfinite(x));CHECK(std::abs(x)<=1);}
void deviceDerivatives(){
    for(double ve:{-.1,0.,.05,.15,.3,.5})for(double vc:{-9.,-2.,-.1,0.,.1}){
        GermaniumDevice p;const auto m=germanium_detail::device(ve,vc,p);CHECK(m.valid);
        const auto a=germanium_detail::device(ve+1e-7,vc,p),b=germanium_detail::device(ve-1e-7,vc,p);
        const auto c=germanium_detail::device(ve,vc+1e-7,p),d=germanium_detail::device(ve,vc-1e-7,p);
        const double actual[]={m.collectorE,m.collectorC,m.baseE,m.baseC};
        const double numeric[]={(a.collector-b.collector)/2e-7,(c.collector-d.collector)/2e-7,(a.base-b.base)/2e-7,(c.base-d.base)/2e-7};
        for(unsigned i=0;i<4;++i)CHECK(std::abs(actual[i]-numeric[i])<1e-8+std::abs(actual[i])*1e-6);
    }
}
void configuration(){
    for(const auto* name:{"","Healthy","unknown"}){bool rejected=false;try{germaniumlab::variation(name);}catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);}
    Germanium a,b;CHECK(a.process(.01f)==b.process(.01f));
    for(float bad:{NAN,INFINITY,-1.f,1.1f})for(bool volume:{false,true}){GermaniumControls c;(volume?c.volume:c.fuzz)=bad;
        CHECK(!a.configure(48000,c));CHECK(a.process(.02f)==b.process(.02f));}
    for(float rate:{NAN,INFINITY,0.f,31999.f,96001.f})CHECK(!a.configure(rate));
    CHECK(!a.configure(48000,{}, {},static_cast<GermaniumRate>(99)));
    struct Limit {double GermaniumCircuit::*field;double low,high;};
    const Limit limits[]={{&GermaniumCircuit::supplyVolts,6,9},{&GermaniumCircuit::sourceOhms,100,100e3},
        {&GermaniumCircuit::inputFarads,1e-6,4.7e-6},{&GermaniumCircuit::collector1Ohms,20e3,50e3},
        {&GermaniumCircuit::feedbackOhms,10e3,150e3},{&GermaniumCircuit::fuzzOhms,500,2000},
        {&GermaniumCircuit::bypassFarads,10e-6,47e-6},{&GermaniumCircuit::collector2Ohms,4.7e3,22e3},
        {&GermaniumCircuit::supplyTapOhms,220,1000},{&GermaniumCircuit::outputFarads,5e-9,22e-9},
        {&GermaniumCircuit::volumeOhms,250e3,1e6},{&GermaniumCircuit::loadOhms,100e3,2e6}};
    for(const auto& l:limits){
        for(double bad:{double(NAN),double(INFINITY),std::nextafter(l.low,0.),std::nextafter(l.high,INFINITY)}){GermaniumCircuit c;c.*(l.field)=bad;
            CHECK(!a.configure(48000,{},c));CHECK(a.process(.01f)==b.process(.01f));}
        for(double endpoint:{l.low,l.high}){GermaniumCircuit c;c.*(l.field)=endpoint;Germanium m;CHECK(m.configure(48000,{},c));
            for(int i=0;i<1024;++i)bounded(m.process(.01f*static_cast<float>(std::sin(i*.12))));
            CHECK(m.failedSolves()==0);}
    }
    struct DeviceLimit {double GermaniumDevice::*field;double low,high;};
    for(const auto& l:{DeviceLimit{&GermaniumDevice::forwardGain,40,300},DeviceLimit{&GermaniumDevice::reverseGain,3,25},
        DeviceLimit{&GermaniumDevice::saturationAmps,10e-6,30e-6},DeviceLimit{&GermaniumDevice::beLeakageAmps,0,25e-6},DeviceLimit{&GermaniumDevice::bcLeakageAmps,0,100e-6}}){
        for(bool second:{false,true})for(double bad:{double(NAN),double(INFINITY),l.low-1e-9,l.high+1e-9}){GermaniumCircuit c;
            auto& selected=second?c.q2:c.q1;selected.*(l.field)=bad;
            CHECK(!a.configure(48000,{},c));CHECK(a.process(.01f)==b.process(.01f));}
        for(bool second:{false,true})for(double value:{l.low,l.high}){GermaniumCircuit c;
            auto& selected=second?c.q2:c.q1;selected.*(l.field)=value;Germanium m;CHECK(m.configure(48000,{},c));
            for(int i=0;i<512;++i)bounded(m.process(.005f*static_cast<float>(std::sin(i*.12))));}}
}
void identityAndSafety(){
    std::array<float,1024> input{},actual{},expected{};for(unsigned i=0;i<input.size();++i)input[i]=static_cast<float>(.02*std::sin(i*.13));
    for(auto mode:{GermaniumRate::Reference1x,GermaniumRate::Oversampled4x})for(const auto* name:{"healthy","q1_leaky","feedback_wrong"}){
        Germanium a,b;const auto c=germaniumlab::variation(name);CHECK(a.configure(44100,{},c,mode));CHECK(b.configure(44100,{},c,mode));
        for(unsigned i=0;i<input.size();++i)expected[i]=a.process(input[i]);
        actual=input;
        b.processBlock(actual.data(),actual.data(),0);b.processBlock(actual.data(),actual.data(),17);b.processBlock(actual.data()+17,actual.data()+17,actual.size()-17);
        CHECK(actual==expected);b.reset();b.processBlock(input.data(),actual.data(),actual.size());CHECK(actual==expected);
        for(float bad:{NAN,INFINITY,-INFINITY}){CHECK(a.process(bad)==0);CHECK(a.process(.01f)==b.process(.01f));}
        CHECK(a.configure(44100,{}, {},mode));CHECK(b.configure(44100,{}, {},mode));
        for(unsigned i=0;i<input.size();++i)CHECK(a.process(input[i])==b.process(input[i]));
    }
    for(float rate:{32000.f,48000.f,96000.f})for(float fuzz:{0.f,.5f,1.f})for(const auto* name:{"healthy","q1_leaky","feedback_wrong"}){
        Germanium m;CHECK(m.configure(rate,{fuzz,.5f},germaniumlab::variation(name)));
        for(int i=0;i<128;++i)CHECK(std::abs(m.process(0))<1e-7);
        watching=true;for(int i=0;i<2048;++i)bounded(m.process(.05f*static_cast<float>(std::sin(i*.13))));watching=false;
        CHECK(allocations==0);CHECK(m.failedSolves()==0&&m.containedOutputs()==0);CHECK(m.maximumLoopEvaluations()<=25);
        m.reset();const float tiny=m.process(std::numeric_limits<float>::denorm_min());m.reset();CHECK(tiny==m.process(0));
        CHECK(m.configure(rate,{fuzz,0},germaniumlab::variation(name)));for(int i=0;i<128;++i)CHECK(m.process(.05f)==0);
    }
    std::uint32_t random=0x003;
    for(const auto* name:{"healthy","q1_leaky","feedback_wrong"}){Germanium m;CHECK(m.configure(48000,{},germaniumlab::variation(name)));
        watching=true;for(unsigned i=0;i<4096;++i){random=random*1664525u+1013904223u;float x;std::memcpy(&x,&random,4);
            bounded(m.process(x));bounded(m.process(i%2?std::numeric_limits<float>::max():-std::numeric_limits<float>::max()));}watching=false;CHECK(allocations==0);
        // Recovery without resetting after overload (fallback may be necessary).
        for(unsigned i=0;i<48000*2;++i)bounded(m.process(0));
        CHECK(std::abs(m.process(0))<1e-6);
        m.reset();for(unsigned i=0;i<48000*2;++i)bounded(m.process(.05f));
        CHECK(std::abs(m.process(.05f))<1e-6);
        for(unsigned i=0;i<48000*2;++i)bounded(m.process(0));
        CHECK(std::abs(m.process(0))<1e-6);CHECK(m.failedSolves()==0);
    }
    // A deliberately underresolved scientific1x diagnostic, not a listening case.
    // Keep its counters visible; do not interpret numerical fallback as aliasing.
    Germanium coarse,fine;CHECK(coarse.configure(48000,{1,.5f},{},GermaniumRate::Reference1x));CHECK(fine.configure(48000,{1,.5f}));
    for(unsigned i=0;i<20480;++i){const float x=.05f*static_cast<float>(std::sin(2*germaniumlab::pi*751*i/4096));
        bounded(coarse.process(x));bounded(fine.process(x));}
    CHECK(coarse.failedSolves()<=coarse.solverCalls());CHECK(coarse.maximumLoopEvaluations()<=25);
    CHECK(fine.failedSolves()==0&&fine.containedOutputs()==0);
    std::cout<<"Underresolved1x diagnostic fallback count "<<coarse.failedSolves()<<" of "<<coarse.solverCalls()<<'\n';
}
int main(){deviceDerivatives();configuration();identityAndSafety();const auto e=germaniumlab::references();
    const auto coarse=germaniumlab::references(96000,false);
    CHECK(e.nodeRms<coarse.nodeRms*.7&&e.nodePeak<coarse.nodePeak*.7);
    std::cout<<"Ge checks "<<testChecks<<" allocations "<<allocations<<" bias peak "<<e.biasPeak<<" nodes RMS/peak "<<e.nodeRms<<'/'<<e.nodePeak
             <<" jack RMS/peak "<<e.jackRms<<'/'<<e.jackPeak<<" input RMS "<<e.inputRms<<'\n';}
