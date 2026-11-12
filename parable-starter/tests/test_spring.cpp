#include "parable_spring.h"
#include "spring_contact.h"
#include "activation_guard.h"
#include "test_check.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
using parable::dsp::Spring;
using parable::dsp::SpringMode;
namespace g=activation_guard;
constexpr double pi=3.14159265358979323846;
static std::vector<std::string> fields(const std::string& s){std::stringstream in(s);std::vector<std::string> f;std::string v;while(std::getline(in,v,','))f.push_back(v);return f;}
static void references() {
    std::ifstream contact(std::string(SPRING_FIXTURES)+"/contact.csv");CHECK(bool(contact));std::string line;std::getline(contact,line);
    unsigned cases=0;
    while(std::getline(contact,line)) {
        const auto f=fields(line);CHECK(f.size()==16);std::array<SpringMode,3> m{};
        const double d[]={.3,-.1,.02},c[]={2,-1,3};double before=0;
        for(unsigned i=0;i<3;++i){m[i].x=std::stod(f[1+i]);m[i].v=std::stod(f[4+i]);m[i].displacement=d[i];m[i].contact=c[i];m[i].omega=i==2?20000:1000;before+=.5*(m[i].x*m[i].x+m[i].v*m[i].v);}
        const auto q=parable::dsp::spring_detail::contact(m,.01,.55,.1004,14);
        double after=0;for(unsigned i=0;i<3;++i) {
            CHECK(std::abs(m[i].x-std::stod(f[7+i]))<1e-16);CHECK(std::abs(m[i].v-std::stod(f[10+i]))<1e-16);
            after+=.5*(m[i].x*m[i].x+m[i].v*m[i].v);
        }
        CHECK(after<=before+1e-16);CHECK(std::abs(before-after-q.projectionLoss-q.restitutionLoss)<1e-16);
        CHECK(std::abs(q.projectionLoss-std::stod(f[13]))<1e-16);CHECK(std::abs(q.restitutionLoss-std::stod(f[14]))<1e-16);
        CHECK(std::abs(q.highGain-std::stod(f[15]))<1e-16);
        if(q.impacted){CHECK(q.before>.01);CHECK(q.velocityBefore>0);CHECK(std::abs(q.velocityAfter+.55*q.velocityBefore)<1e-16);CHECK(q.highGain>0);}
        ++cases;
    }
    CHECK(cases==4);
    std::vector<double> errors;
    for(unsigned rate:{32000u,44100u,48000u,96000u}) {
        Spring h,f;CHECK(h.prepare(float(rate),{false,1,1}));CHECK(f.prepare(float(rate),{true,1,1}));
        std::ifstream reference(std::string(SPRING_FIXTURES)+"/linear.csv");CHECK(bool(reference));std::getline(reference,line);
        size_t n=0,count=0;double sum=0,peak=0;
        while(std::getline(reference,line)) {
            auto values=fields(line);const size_t end=size_t(std::llround(std::stod(values[0])*rate));
            for(;n<end;++n) {
                const float x=n<rate/200?float(.05*std::sin(pi*(double(n)+.5)/rate/.005)):0;
                g::begin();h.process(x);f.process(x);g::end();CHECK(g::clean());
                CHECK(h.secondaryPickup()==f.secondaryPickup()); // unchanged physical path, no port limiting.
            }
            const double e=std::abs(h.secondaryPickup()-std::stod(values[1]));sum+=e*e;peak=std::max(peak,e);++count;
        }
        errors.push_back(std::sqrt(sum/count));CHECK(peak<4e-6&&errors.back()<8e-7);CHECK(f.inputLimits()==0&&f.numericalFaults()==0);
        std::cout<<"Linear independent convolution rate="<<rate<<" rms="<<errors.back()<<" peak="<<peak<<'\n';
    }
    CHECK(errors.back()<.7*errors.front());
}
static void physics() {
    std::vector<double> projectionByRate,hfByRate,onsetByRate;
    for(unsigned rate:{32000u,44100u,48000u,96000u})for(float amplitude:{.05f,.8f}) {
        Spring h,f;CHECK(h.prepare(float(rate),{false,1,1}));CHECK(f.prepare(float(rate),{true,1,1}));
        double loss=0,projection=0,hf=0,balance=0,penetration=0,releaseEnergy=0;bool differs=false;double onset=-1;
        for(unsigned n=0;n<rate;++n) {
            const float x=n<rate/200?float(amplitude*std::sin(pi*(double(n)+.5)/rate/.005)):0;
            g::begin();const auto a=h.process(x),b=f.process(x);g::end();CHECK(g::clean());
            CHECK(std::isfinite(a)&&std::isfinite(b)&&std::abs(a)<=.800001&&std::abs(b)<=.800001);
            const auto& q=f.probe();loss+=q.restitutionLossJ;projection+=q.projectionLossJ;hf=std::max(hf,q.maximumImpactHighGainJ);
            balance=std::max(balance,std::abs(q.balanceErrorJ));penetration=std::max(penetration,q.penetrationM);
            CHECK(q.energyEndJ<=Spring::kEnergyLimitJ*(1+1e-10));CHECK(q.restitutionLossJ>=0&&q.projectionLossJ>=0);
            CHECK(q.displacementAfterM<=Spring::kFaultGapM+1e-15);
            if(q.impacts){if(onset<0)onset=double(n+1)/rate;CHECK(q.displacementBeforeM>Spring::kFaultGapM);CHECK(q.contactVelocityBefore>0);CHECK(std::abs(q.contactVelocityAfter+.55*q.contactVelocityBefore)<1e-12);}
            if(n==rate/200-1)releaseEnergy=q.energyEndJ;
            if(n>=rate/200)CHECK(q.energyEndJ<=releaseEnergy*std::exp(-6*double(n-rate/200+1)/rate)*(1+1e-9));
            CHECK(h.secondaryPickup()==f.secondaryPickup());differs|=a!=b;
        }
        CHECK(h.impacts()==0&&h.inputLimits()==0&&f.inputLimits()==0);CHECK(h.numericalFaults()==0&&f.numericalFaults()==0);CHECK(balance<1e-18);
        if(amplitude<.1)CHECK(f.impacts()==0&&!differs);
        else {CHECK(f.impacts()>10&&differs&&hf>1e-12);CHECK(projection<.3*loss);CHECK(penetration<.4e-6);projectionByRate.push_back(projection);hfByRate.push_back(hf);onsetByRate.push_back(onset);}
        std::cout<<"Contact rate="<<rate<<" amplitude="<<amplitude<<" impacts="<<f.impacts()<<" restitution_J="<<loss<<" projection_J="<<projection<<" penetration_m="<<penetration<<" peak_HF_gain_J="<<hf<<" onset_s="<<onset<<" ledger_error_J="<<balance<<'\n';
    }
    CHECK(projectionByRate.back()<.6*projectionByRate.front());
    CHECK(*std::max_element(hfByRate.begin(),hfByRate.end())-*std::min_element(hfByRate.begin(),hfByRate.end())<.002*hfByRate.front());
    CHECK(*std::max_element(onsetByRate.begin(),onsetByRate.end())-*std::min_element(onsetByRate.begin(),onsetByRate.end())<2./32000);
}
static void safety() {
    Spring s,a,b;CHECK(s.process(1)==0);CHECK(sizeof(Spring)<=24576);g::selfTest();
    for(unsigned rate:{32000u,44100u,48000u,96000u}) {
        CHECK(a.prepare(float(rate),{true,.4f,.7f}));CHECK(b.prepare(float(rate),{true,.4f,.7f}));
        for(unsigned n=0;n<200;++n)CHECK(a.process(.1f)==b.process(.1f));
        for(float v:{NAN,INFINITY,0.f,31999.f,96001.f})CHECK(!a.prepare(v));
        for(float v:{NAN,INFINITY,-.1f,1.1f}){CHECK(!a.prepare(float(rate),{false,v,1}));CHECK(!a.prepare(float(rate),{false,1,v}));}
        for(unsigned n=0;n<200;++n)CHECK(a.process(.1f)==b.process(.1f));
        a.stop();CHECK(a.process(1)==0);a.reset();b.reset();for(unsigned n=0;n<200;++n)CHECK(a.process(.1f)==b.process(.1f));
        const float edges[]={NAN,INFINITY,-INFINITY,std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),0};
        for(float mix:{0.f,1.f})for(float ret:{0.f,1.f}) {
            CHECK(s.prepare(float(rate),{true,mix,ret}));for(unsigned n=0;n<2000;++n) {
                const float x=edges[n%6];g::begin();const auto y=s.process(x);g::end();CHECK(g::clean());CHECK(std::isfinite(y)&&std::abs(y)<=1);
                if(mix==0)CHECK(y==(std::isfinite(x)?std::max(-1.f,std::min(1.f,x)):0));
                if(mix==1&&ret==0)CHECK(y==0);
            }CHECK(s.numericalFaults()==0);
        }
        CHECK(s.prepare(float(rate),{true,1,1}));for(unsigned n=0;n<100;++n)CHECK(s.process(0)==0);
        for(unsigned n=0;n<rate/100;++n)s.process(float(.8*std::sin(pi*n/(rate/100))));
        double last=0;for(unsigned n=0;n<6*rate;++n)last=s.process(0);
        CHECK(last==0&&s.tailClears()==1&&s.energyJ()==0&&s.probe().tailDiscardedJ<1e-20);
        for(unsigned n=0;n<500;++n)CHECK(s.process(0)==0);
        CHECK(s.prepare(float(rate),{false,1,1}));CHECK(s.displacementBoundM()<Spring::kHealthyGapM);
        // Adversarial resonant forcing exercises the explicit port budget.
        for(unsigned n=0;n<rate;++n){g::begin();const auto y=s.process(float(std::sin(2*pi*26*n/rate)));g::end();CHECK(g::clean());CHECK(std::isfinite(y)&&std::abs(y)<=.800001);}
        CHECK(s.inputLimits()>0&&s.numericalFaults()==0);
    }
    std::cout<<"Spring state="<<sizeof(Spring)<<"; finite return/energy/tail/reset/extremes and callback guards passed\n";
}
int main(){references();physics();safety();std::cout<<"Spring checks="<<testChecks<<" passed\n";}
