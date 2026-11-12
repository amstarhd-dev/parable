#include "parable_rotary.h"
#include "rotary_delay.h"
#include "activation_guard.h"
#include "test_check.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
using parable::dsp::Rotary;
using parable::dsp::RotaryConfig;
namespace g=activation_guard;
constexpr double pi=3.14159265358979323846;
static std::vector<std::string> fields(const std::string& line) {
    std::stringstream in(line);std::vector<std::string> out;std::string value;
    while(std::getline(in,value,','))out.push_back(value);
    return out;
}
static float command(size_t n,unsigned rate) {return n>=rate&&n<5*rate?1.f:0.f;}
static double angleError(double a,double b) {return std::abs(std::remainder(a-b,2*pi));}
static void reference() {
    std::vector<double> rmsErrors;
    for(unsigned rate:{32000u,44100u,48000u,96000u}) {
        std::ifstream file(std::string(ROTARY_FIXTURES)+"/mechanics.csv");CHECK(bool(file));std::string line,current;std::getline(file,line);
        Rotary model;size_t sample=0,count=0;double velocity=0,phase=0,sum=0;std::vector<std::pair<std::string,double>> events;
        while(std::getline(file,line)) {
            const auto f=fields(line);CHECK(f.size()==11);const auto key=f[0]+f[1];const bool fault=f[1]!="0";
            if(key!=current) {current=key;sample=0;CHECK(model.prepare(float(rate),{fault,1}));}
            const size_t end=size_t(std::llround(std::stod(f[2])*rate));
            for(;sample<end;++sample) {
                g::begin();const auto y=model.process(0,command(sample,rate));g::end();CHECK(g::clean());CHECK(y==0);
                if(fault) {if(model.horn().released)events.emplace_back("release",double(sample+1)/rate);if(model.horn().captured)events.emplace_back("capture",double(sample+1)/rate);}
            }
            const auto& s=f[0]=="horn"?model.horn():model.drum();
            for(auto pair:{std::pair<double,double>{s.motorOmega,std::stod(f[3])},{s.omega,std::stod(f[4])}}) {
                const double e=std::abs(pair.first-pair.second);velocity=std::max(velocity,e);sum+=e*e;++count;
            }
            phase=std::max({phase,angleError(s.motorAngle,std::stod(f[5])),angleError(s.angle,std::stod(f[6])),angleError(s.beltPhase,std::stod(f[7]))});
            CHECK(model.containmentEvents()==0);
        }
        CHECK(count==4800);CHECK(velocity<.1);CHECK(phase<.02);rmsErrors.push_back(std::sqrt(sum/count));
        std::ifstream expected(std::string(ROTARY_FIXTURES)+"/events.csv");CHECK(bool(expected));std::getline(expected,line);size_t event=0;double timing=0;
        while(std::getline(expected,line)){const auto f=fields(line);CHECK(event<events.size());CHECK(f[0]==events[event].first);timing=std::max(timing,std::abs(events[event].second-std::stod(f[1])));++event;}
        CHECK(event==events.size()&&event>=10);CHECK(timing<.002);
        std::cout<<"Mechanics rate="<<rate<<" values="<<count<<" omega_peak/rms="<<velocity<<'/'<<rmsErrors.back()<<" phase_peak="<<phase<<" event_peak_s="<<timing<<" events="<<event<<'\n';
    }
    CHECK(rmsErrors.back()<.8*rmsErrors.front()); // sample-rate refinement, not only bounded output.
}
static void interpolation() {
    std::array<float,384> memory{};
    for(unsigned current:{0u,1u,127u,383u}) {
        for(unsigned age=0;age<384;++age)memory[(current+384-age)%384]=float(age)/1024;
        for(double delay:{1.,1.5,37.25,330.999,331.})CHECK(std::abs(parable::dsp::rotary_detail::read(memory.data(),current,delay)-delay/1024)<3e-8);
    }
    // Known half-sample linear-interpolator response, including HF loss.
    for(unsigned bin:{8u,64u,128u,160u}) {
        for(unsigned i=0;i<384;++i)memory[i]=float(std::sin(2*pi*bin*i/384));
        double energy=0;for(unsigned i=0;i<384;++i){const auto y=parable::dsp::rotary_detail::read(memory.data(),i,1.5);energy+=y*y;}
        const double amplitude=std::sqrt(2*energy/384),expected=std::cos(pi*bin/384);
        CHECK(std::abs(amplitude-expected)<1e-7);
    }
}
static void safety() {
    g::selfTest();Rotary healthy,fault,copy;
    CHECK(healthy.process(1,1)==0);
    double difference=0,minHorn=100;size_t releases=0,captures=0;float maxDelta=0;
    for(unsigned rate:{32000u,44100u,48000u,96000u}) {
        CHECK(healthy.prepare(float(rate),{false,1}));CHECK(fault.prepare(float(rate),{true,1}));
        float previous=0;double previousAngle=0,previousDelay=0;
        for(size_t n=0;n<12*rate;++n) {
            const float x=float(.2*std::sin(2*pi*997*n/rate));
            g::begin();const float h=healthy.process(x,command(n,rate)),f=fault.process(x,command(n,rate));g::end();CHECK(g::clean());
            CHECK(std::isfinite(h)&&std::isfinite(f)&&std::abs(h)<=1&&std::abs(f)<=1);
            CHECK(fault.drum().omega==healthy.drum().omega&&fault.drum().angle==healthy.drum().angle);
            CHECK(fault.drumContribution()==healthy.drumContribution());
            CHECK(angleError(fault.horn().angle,previousAngle)<=80./rate+1e-12);previousAngle=fault.horn().angle;
            CHECK(fault.hornDelaySamples()>1 && fault.hornDelaySamples()<Rotary::kDelayCells-2);
            if(n)CHECK(std::abs(fault.hornDelaySamples()-previousDelay)<.043);
            previousDelay=fault.hornDelaySamples();
            const double exact=rate*(1-.18*std::cos(fault.horn().angle))/343;
            CHECK(std::abs(exact-fault.hornDelaySamples())<2e-6); // Hermite-table geometry at96k.
            CHECK(fault.horn().captureLossJ>=-1e-12);
            CHECK(!healthy.horn().slipping&&!healthy.drum().slipping&&!fault.drum().slipping);
            releases+=fault.horn().released;captures+=fault.horn().captured;
            if(n>6*rate)minHorn=std::min(minHorn,fault.horn().omega);
            difference+=double(h-f)*(h-f);if(n)maxDelta=std::max(maxDelta,std::abs(f-previous));previous=f;
        }
        CHECK(fault.containmentEvents()==0&&healthy.containmentEvents()==0);
        // Rejected preparation preserves working audio/phase histories.
        CHECK(healthy.prepare(float(rate),{}));CHECK(copy.prepare(float(rate),{}));
        for(unsigned n=0;n<100;++n)CHECK(healthy.process(.1f,1)==copy.process(.1f,1));
        for(float bad:{NAN,INFINITY,0.f,31999.f,96001.f})CHECK(!healthy.prepare(bad,{}));
        CHECK(!healthy.prepare(float(rate),{false,NAN}));CHECK(!healthy.prepare(float(rate),{false,-.1f}));CHECK(!healthy.prepare(float(rate),{false,1.1f}));
        for(unsigned n=0;n<1000;++n)CHECK(healthy.process(.1f,1)==copy.process(.1f,1));
        healthy.stop();const auto angle=healthy.horn().angle;CHECK(healthy.process(1,1)==0);CHECK(healthy.horn().angle==angle);
        healthy.reset();copy.reset();for(unsigned n=0;n<1000;++n)CHECK(healthy.process(.1f,1)==copy.process(.1f,1));
        const float edges[]={NAN,INFINITY,-INFINITY,std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),0};
        for(float mix:{0.f,1.f}) {CHECK(fault.prepare(float(rate),{true,mix}));
            for(unsigned n=0;n<rate;++n){const auto y=fault.process(edges[n%6],n%2?INFINITY:-1);CHECK(std::isfinite(y)&&std::abs(y)<=1);}
            CHECK(fault.containmentEvents()==0);
        }
    }
    CHECK(releases>=20&&captures>=20&&difference>.1&&minHorn<3);CHECK(maxDelta<.06);
    // A constant source isolates control-induced discontinuities from an audio
    // sine's natural slope. Exclude only the initial propagation/filter onset.
    CHECK(fault.prepare(48000,{true,1}));float previous=0,dcStep=0;
    for(size_t n=0;n<12*48000;++n){const auto y=fault.process(.25f,command(n,48000));if(n>960)dcStep=std::max(dcStep,std::abs(y-previous));previous=y;}
    CHECK(dcStep<.0005f);CHECK(fault.containmentEvents()==0);
    CHECK(sizeof(Rotary)<=5120);
    std::cout<<"Rotary state="<<sizeof(Rotary)<<" repeated releases/captures="<<releases<<'/'<<captures<<" slow minimum_rad_s="<<minHorn<<" 997Hz max_sample_delta="<<maxDelta<<" DC max_delta="<<dcStep<<"; bass exact; callback guards clean\n";
}
int main(){reference();interpolation();safety();std::cout<<"Rotary checks="<<testChecks<<" passed\n";}
