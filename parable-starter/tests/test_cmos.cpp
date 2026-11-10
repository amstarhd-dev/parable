#include "cmos_measure.h"
#include "test_check.h"
#include <cstring>
#include <limits>
#include <new>
#include <iostream>
using namespace parable::dsp;
static bool watching=false;static std::size_t allocations=0;
void* operator new(std::size_t n){if(watching)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void bounded(float x){CHECK(std::isfinite(x));CHECK(std::abs(x)<=1);}
void configuration(){
    for(const auto* name:{"healthy","coupling_small","feedback_low"}){const auto c=cmoslab::variation(name);Cmos m;CHECK(m.configure(48000,{},c));}
    for(const auto* name:{"","other","Healthy"}){bool rejected=false;try{cmoslab::variation(name);}catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);}
    Cmos a,b;CHECK(a.process(.01f)==b.process(.01f));
    for(float bad:{NAN,INFINITY,-1.f,1.1f})for(int control=0;control<2;++control){CmosControls c;
        (control?c.volume:c.drive)=bad;CHECK(!a.configure(48000,c));CHECK(a.process(.1f)==b.process(.1f));}
    for(float rate:{NAN,INFINITY,0.f,31999.f,96001.f})CHECK(!a.configure(rate));
    CHECK(!a.configure(48000,{}, {},static_cast<CmosRate>(99)));
    for(int field=0;field<12;++field){CmosCircuit c;
        switch(field){case 0:c.railVolts=5;break;case 1:c.inputOhms=0;break;case 2:c.inputFarads=NAN;break;
        case 3:c.firstFeedbackOhms=INFINITY;break;case 4:c.drivePotOhms=-1;break;case 5:c.firstFeedbackFarads=0;break;
        case 6:c.interstageFarads=0;break;case 7:c.secondFeedbackOhms=0;break;case 8:c.secondFeedbackFarads=NAN;break;
        case 9:c.outputFarads=INFINITY;break;case 10:c.volumePotOhms=0;break;case 11:c.loadOhms=-1;break;}
        CHECK(!a.configure(48000,{},c));CHECK(a.process(-.1f)==b.process(-.1f));}
}
void identity(){
    std::array<float,777> input{},expected{},actual{};
    for(std::size_t i=0;i<input.size();++i)input[i]=static_cast<float>(.1*std::sin(i*.13));
    for(auto mode:{CmosRate::Reference1x,CmosRate::Oversampled4x}){
        Cmos a,b;CHECK(a.configure(44100,{}, {},mode));CHECK(b.configure(44100,{}, {},mode));
        for(std::size_t i=0;i<input.size();++i)expected[i]=a.process(input[i]);
        actual=input;b.processBlock(actual.data(),actual.data(),0);b.processBlock(actual.data(),actual.data(),17);
        b.processBlock(actual.data()+17,actual.data()+17,actual.size()-17);CHECK(actual==expected);
        b.reset();b.processBlock(input.data(),actual.data(),actual.size());CHECK(actual==expected);
        for(float bad:{NAN,INFINITY,-INFINITY}){CHECK(a.process(bad)==0);CHECK(a.process(.02f)==b.process(.02f));}
    }
    Cmos reused;
    for(const auto* name:{"coupling_small","feedback_low","healthy"}){for(int i=0;i<100;++i)reused.process(.1f);
        Cmos fresh;const auto c=cmoslab::variation(name);CHECK(reused.configure(48000,{},c));CHECK(fresh.configure(48000,{},c));
        for(int i=0;i<1000;++i)CHECK(reused.process(input[i%input.size()])==fresh.process(input[i%input.size()]));}
}
void parameterLimits(){
    struct Limit {double CmosCircuit::*field;double low,high;};
    const Limit limits[]{
        {&CmosCircuit::railVolts,6,9},{&CmosCircuit::inputOhms,50e3,200e3},
        {&CmosCircuit::inputFarads,10e-9,220e-9},{&CmosCircuit::firstFeedbackOhms,50e3,200e3},
        {&CmosCircuit::drivePotOhms,100e3,1e6},{&CmosCircuit::firstFeedbackFarads,25e-12,200e-12},
        {&CmosCircuit::interstageFarads,330e-12,100e-9},{&CmosCircuit::secondFeedbackOhms,100e3,2e6},
        {&CmosCircuit::secondFeedbackFarads,50e-12,220e-12},{&CmosCircuit::outputFarads,1e-6,22e-6},
        {&CmosCircuit::volumePotOhms,5e3,20e3},{&CmosCircuit::loadOhms,100e3,2e6}};
    for(const auto& limit:limits){
        Cmos a,b;
        for(double bad:{std::nextafter(limit.low,0.),std::nextafter(limit.high,INFINITY)}){
            CmosCircuit c;c.*(limit.field)=bad;CHECK(!a.configure(48000,{},c));CHECK(a.process(.01f)==b.process(.01f));}
        for(double value:{limit.low,limit.high})for(float rate:{32000.f,96000.f})for(float drive:{0.f,1.f}){
            CmosCircuit c;c.*(limit.field)=value;Cmos m;CHECK(m.configure(rate,{drive,1},c));
            for(int i=0;i<512;++i)bounded(m.process(.02f*static_cast<float>(std::sin(i*.19))));
            CHECK(m.failedSolves()==0);CHECK(m.maximumLoopEvaluations()<=16);}
    }
}
void safety(){
    std::uint32_t random=0x4049;
    for(float rate:{32000.f,48000.f,96000.f})for(const auto* name:{"healthy","coupling_small","feedback_low"}){
        Cmos r;CHECK(r.configure(rate,{1,1},cmoslab::variation(name)));
        for(int i=0;i<256;++i)CHECK(std::abs(r.process(0))<1e-8);
        r.reset();const float tiny=r.process(std::numeric_limits<float>::denorm_min());r.reset();CHECK(r.process(0)==tiny);
        watching=true;
        for(int i=0;i<4096;++i){random=random*1664525u+1013904223u;float value;std::memcpy(&value,&random,4);
            bounded(r.process(value));bounded(r.process(i%2?std::numeric_limits<float>::max():-std::numeric_limits<float>::max()));}
        watching=false;CHECK(allocations==0);
        r.reset();for(int i=0;i<2000;++i)bounded(r.process(.2f*static_cast<float>(std::sin(i*.17))));
        CHECK(r.failedSolves()==0);
        CHECK(r.configure(rate,{.5f,0},cmoslab::variation(name)));for(int i=0;i<256;++i)CHECK(r.process(.2f)==0);
    }
    Cmos r;CHECK(r.configure(48000,{},cmoslab::variation("coupling_small"),CmosRate::Reference1x));
    for(int i=0;i<48000*5;++i)bounded(r.process(.5f));
    CHECK(std::abs(r.process(.5f))<1e-7);
    for(int i=0;i<48000*5;++i)bounded(r.process(0));
    CHECK(std::abs(r.process(0))<1e-7);
    CHECK(r.failedSolves()==0);
    for(double rail:{6.,9.}){CmosCircuit c;c.railVolts=rail;Cmos m;CHECK(m.configure(96000,{0,1},c));
        for(int i=0;i<2000;++i)bounded(m.process(.1f*static_cast<float>(std::sin(i*.13))));
        CHECK(m.failedSolves()==0);}
}
int main(){configuration();identity();parameterLimits();safety();const auto e=cmoslab::references();
    std::cout<<"CMOS checks "<<testChecks<<" allocation count "<<allocations<<" DC peak V "<<e.dcPeak
             <<" circuit RMS/peak V "<<e.transientRms<<'/'<<e.transientPeak<<'\n';}
