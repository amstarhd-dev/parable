#include "parable_muff.h"
#include "muff_device.h"
#include "muff_stimulus.h"
#include "activation_guard.h"
#include "test_check.h"
#include <algorithm>
#include <complex>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
using parable::dsp::Muff;
using parable::dsp::MuffConfig;
using parable::dsp::MuffRate;
namespace device=parable::dsp::muff_detail;
namespace g=activation_guard;
static std::vector<std::vector<std::string>> csv(const char* name) {
    std::ifstream f(std::string(MUFF_FIXTURES)+"/"+name);CHECK(bool(f));std::string line;std::getline(f,line);std::vector<std::vector<std::string>> rows;
    while(std::getline(f,line)){std::stringstream in(line);std::string s;std::vector<std::string> row;while(std::getline(in,s,','))row.push_back(s);rows.push_back(row);}return rows;
}
static double val(const std::vector<std::string>& r,size_t n){return std::stod(r[n]);}
static void devicesAndBias() {
    size_t points=0;double error=0;
    for(const auto& r:csv("device.csv")) {
        Muff::Ports v{};v[0]=val(r,0);v[1]=val(r,1);const auto d=device::currents(v);CHECK(d.valid);
        const double base=d.i[0]/device::bf+d.i[1]/device::br,col=d.i[0]-(1+1/device::br)*d.i[1];
        for(auto pair:{std::pair<double,double>{base,val(r,2)},{col,val(r,3)}}){const auto e=std::abs(pair.first-pair.second);error=std::max(error,e);CHECK(e<1e-10+2e-5*std::abs(pair.second));}
        ++points;
    }CHECK(points==72);
    for(const auto& r:csv("diodes.csv")){Muff::Ports v{};v[8]=val(r,0);const auto d=device::currents(v);CHECK(d.valid);CHECK(std::abs(d.i[8]+device::diodeG0*v[8]-val(r,1))<1e-11+2e-5*std::abs(val(r,1)));}
    for(size_t p:{size_t(0),size_t(1),size_t(8)})for(double voltage:{-.6,-.1,0.,.1,.6}) {
        Muff::Ports v{};v[p]=voltage;const auto d=device::currents(v);auto hi=v,lo=v;hi[p]+=1e-7;lo[p]-=1e-7;
        const double derivative=(device::currents(hi).i[p]-device::currents(lo).i[p])/2e-7;
        CHECK(std::abs(derivative-d.g[p])<1e-13+2e-6*std::abs(d.g[p]));
    }
    double biasError=0;
    for(const auto& r:csv("bias.csv")) {
        Muff m;CHECK(m.prepare(48000,{val(r,0)!=0,float(val(r,1)),float(val(r,2)),float(val(r,3))}));
        for(size_t n=0;n<Muff::kNodes;++n)biasError=std::max(biasError,std::abs(m.biasNodes()[n]-val(r,4+n)));
        if(val(r,0)){CHECK(m.biasNodes()[Muff::B2]<.4&&m.biasNodes()[Muff::C2]>8);}
        else CHECK(m.biasNodes()[Muff::C2]>4&&m.biasNodes()[Muff::C2]<5);
    }CHECK(biasError<5e-6);
    std::cout<<"Independent built-in device points="<<points<<" peak_current_A="<<error<<" bias_peak_V="<<biasError<<'\n';
}
static void transients() {
    const auto rows=csv("transient.csv");std::vector<double> rms;
    for(unsigned rate:{96000u,192000u,384000u}) {
        Muff m;std::string group;size_t next=0,count=0,jackCount=0;double sum=0,peak=0,jackSum=0,jackPeak=0,currentPeak=0;
        for(const auto& r:rows) {
            CHECK(r.size()==28);const auto key=r[0]+r[1];const bool fault=val(r,0)!=0,attack=val(r,1)!=0;
            if(key!=group){group=key;next=0;CHECK(m.prepare(float(rate),{fault,.75f,.5f,.5f},MuffRate::Reference1x));}
            const size_t target=size_t(std::llround(val(r,3)*rate));
            for(;next<=target;++next){g::begin();const auto y=m.process(mufflab::history(double(next)/rate,attack));g::end();CHECK(g::clean());CHECK(std::isfinite(y)&&std::abs(y)<=1);}
            for(size_t i=0;i<21;++i) {
                const double actual=i<17?m.nodes()[i]:i==17?m.capacitorVolts(6):i==18?m.capacitorVolts(12):i==19?m.toneVolts():m.jackVolts();
                const double e=std::abs(actual-val(r,4+i));sum+=e*e;peak=std::max(peak,e);++count;
                if(i==20){jackSum+=e*e;jackPeak=std::max(jackPeak,e);++jackCount;}
            }
            for(size_t i=0;i<3;++i){const double actual=i==0?m.inputAmps():m.diodeAmps(i-1);currentPeak=std::max(currentPeak,std::abs(actual-val(r,25+i)));}
            CHECK(m.failures()==0&&m.outputClamps()==0&&m.maximumEvaluations()<=Muff::kUpdates+1);
        }
        rms.push_back(std::sqrt(sum/count));CHECK(peak<.015&&rms.back()<.0015&&currentPeak<5e-6);
        std::cout<<"Transient rate="<<rate<<" values="<<count<<" nodes_rms/peak_V="<<rms.back()<<'/'<<peak<<" jack_rms/peak_V="<<std::sqrt(jackSum/jackCount)<<'/'<<jackPeak<<" current_peak_A="<<currentPeak<<'\n';
    }
    CHECK(rms.back()<.7*rms.front());
}
static void recoveryReference() {
    const auto expected=csv("recovery.csv");double rmsError=0,meanError=0;
    constexpr unsigned rate=192000;const double times[]={.09,.13,.18,.3,.35,.45,.6};
    for(bool fault:{false,true})for(bool attack:{false,true}) {
        Muff m;CHECK(m.prepare(rate,{fault,.75f,.5f,.5f},MuffRate::Reference1x));
        std::array<std::array<double,6>,7> sums{};std::array<unsigned,7> counts{};
        for(unsigned n=0;n<rate*65/100;++n) {
            m.process(mufflab::history(double(n)/rate,attack));
            for(unsigned k=0;k<7;++k)if(double(n)/rate>=times[k]&&double(n)/rate<times[k]+.02) {
                const double values[]={m.jackVolts()*m.jackVolts(),m.nodes()[Muff::B2],m.nodes()[Muff::C2],m.nodes()[Muff::E2],m.capacitorVolts(6),m.capacitorVolts(12)};
                for(unsigned j=0;j<6;++j)sums[k][j]+=values[j];
                ++counts[k];
            }
        }
        for(const auto& r:expected)if((val(r,0)!=0)==fault&&(val(r,1)!=0)==attack) {
            unsigned k=0;while(k<7&&std::abs(times[k]-val(r,2))>1e-10)++k;CHECK(k<7&&counts[k]>0);
            const double actual=std::sqrt(sums[k][0]/counts[k]);rmsError=std::max(rmsError,std::abs(actual-val(r,4))/val(r,4));
            for(unsigned j=1;j<6;++j)meanError=std::max(meanError,std::abs(sums[k][j]/counts[k]-val(r,4+j)));
        }CHECK(m.failures()==0);
    }
    CHECK(rmsError<.005&&meanError<.0005);
    std::cout<<"Independent recovery RMS_relative_error="<<rmsError<<" mean_state_peak_V="<<meanError<<'\n';
}
static void controls() {
    std::array<std::array<double,3>,3> tone{};std::array<double,3> sustain{},volume{};double worst=0;
    for(const auto& r:csv("ac.csv")) {
        Muff m;const double f=val(r,4),amplitude=1e-7;constexpr unsigned rate=192000;
        CHECK(m.prepare(rate,{false,float(val(r,1)),float(val(r,2)),float(val(r,3))},MuffRate::Reference1x));
        std::array<std::complex<double>,3> bins{};unsigned count=0;
        for(unsigned n=0;n<rate*3/10;++n) {
            const double phase=2*mufflab::pi*f*n/rate;
            g::begin();m.process(float(amplitude*std::sin(phase)));g::end();CHECK(g::clean());
            if(n>=rate/5){const std::complex<double> reference(std::sin(phase),std::cos(phase));bins[0]+=(m.nodes()[Muff::C2]-m.biasNodes()[Muff::C2])*reference;bins[1]+=(m.toneVolts()-((1-val(r,2))*m.biasNodes()[Muff::ToneLow]+val(r,2)*m.biasNodes()[Muff::ToneHigh]))*reference;bins[2]+=m.jackVolts()*reference;++count;}
        }
        for(auto& v:bins)v*=2/(count*amplitude);
        for(unsigned k=0;k<3;++k) {
            const std::complex<double> expected(val(r,5+2*k),val(r,6+2*k));
            if(std::abs(expected)>1e-6){const auto e=std::abs(bins[k]-expected)/std::abs(expected);worst=std::max(worst,e);CHECK(e<.05);}
            else CHECK(std::abs(bins[k])<.001);
        }
        const unsigned fi=f==100?0:f==1000?1:2;
        if(r[0]=="tone")tone[unsigned(std::lround(2*val(r,2)))][fi]=std::abs(bins[1]/bins[0]);
        if(f==1000&&r[0]=="sustain")sustain[unsigned(std::lround(2*val(r,1)))]=std::abs(bins[2]);
        if(f==1000&&r[0]=="volume")volume[unsigned(std::lround(2*val(r,3)))]=std::abs(bins[2]);
        CHECK(m.failures()==0&&m.outputClamps()==0);
    }
    CHECK(tone[1][1]<.65*std::sqrt(tone[1][0]*tone[1][2]));CHECK(tone[0][0]>2*tone[2][0]);CHECK(tone[2][2]>2*tone[0][2]);
    CHECK(sustain[0]>0&&sustain[0]<.05*sustain[2]&&sustain[0]<sustain[1]&&sustain[1]<sustain[2]);
    CHECK(volume[0]<1e-6&&volume[2]>1.5*volume[1]&&volume[2]<3*volume[1]);
    std::cout<<"Control AC max_complex_relative_error="<<worst<<" tone_mid_100/1k/10k="<<tone[1][0]<<'/'<<tone[1][1]<<'/'<<tone[1][2]<<" sustain0/mid/1="<<sustain[0]<<'/'<<sustain[1]<<'/'<<sustain[2]<<'\n';
}
static void historyAndSafety() {
    g::selfTest();Muff a,b;CHECK(a.process(1)==0);double ratio=0;
    for(unsigned rate:{44100u,48000u,96000u})for(bool fault:{false,true}) {
        std::array<double,2> rms{};
        for(bool attack:{false,true}) {
            CHECK(a.prepare(float(rate),{fault,.75f,.5f,.5f}));double sum=0;unsigned count=0;
            for(unsigned n=0;n<rate*7/10;++n){g::begin();const auto y=a.process(mufflab::history(double(n)/rate,attack));g::end();CHECK(g::clean());CHECK(std::isfinite(y)&&std::abs(y)<=1);if(double(n)/rate>=.09&&double(n)/rate<.11){sum+=double(y)*y;++count;}}
            rms[attack]=std::sqrt(sum/count);CHECK(a.failures()==0&&a.outputClamps()==0&&a.maximumEvaluations()<=25);
        }
        const double r=rms[1]/rms[0];if(fault){CHECK(r>1.05&&r<1.5);ratio=r;}else CHECK(std::abs(r-1)<.02);
        CHECK(a.prepare(float(rate),{fault,.75f,.5f,.5f}));
        for(unsigned n=0;n<2*rate;++n){g::begin();const auto y=a.process(mufflab::note(double(n)/rate));g::end();CHECK(g::clean());CHECK(std::isfinite(y)&&std::abs(y)<=1);}
        for(unsigned n=0;n<rate/2;++n)CHECK(std::isfinite(a.process(0)));
        CHECK(a.failures()==0&&a.outputClamps()==0);
        CHECK(a.prepare(float(rate),{}));CHECK(b.prepare(float(rate),{}));
        for(unsigned n=0;n<100;++n)CHECK(a.process(.03f)==b.process(.03f));
        for(float bad:{NAN,INFINITY,0.f,31999.f,96001.f})CHECK(!a.prepare(bad));
        for(float bad:{NAN,INFINITY,-.1f,1.1f}){CHECK(!a.prepare(float(rate),{false,bad,.5f,.5f}));CHECK(!a.prepare(float(rate),{false,.5f,bad,.5f}));CHECK(!a.prepare(float(rate),{false,.5f,.5f,bad}));}
        CHECK(!a.prepare(float(rate),{},static_cast<MuffRate>(99)));
        for(unsigned n=0;n<200;++n)CHECK(a.process(.03f)==b.process(.03f));
        a.stop();CHECK(a.process(1)==0);a.reset();b.reset();for(unsigned n=0;n<200;++n)CHECK(a.process(.03f)==b.process(.03f));
        CHECK(a.prepare(float(rate),{fault,.75f,.5f,0}));for(unsigned n=0;n<1000;++n)CHECK(a.process(.2f)==0);
        CHECK(a.prepare(float(rate),{fault,1,1,1}));const float edges[]={NAN,INFINITY,-INFINITY,std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),std::numeric_limits<float>::denorm_min(),0};
        for(unsigned n=0;n<2048;++n){g::begin();const auto y=a.process(edges[n%7]);g::end();CHECK(g::clean());CHECK(std::isfinite(y)&&std::abs(y)<=1);}
        CHECK(a.invalidInputs()>0&&a.inputClamps()>0);
        a.reset();for(unsigned n=0;n<2048;++n)CHECK(std::abs(a.process(0))<1e-8);
    }
    std::cout<<"History priming ratio (NOT suppression)="<<ratio<<" state="<<sizeof(Muff)<<"; guards/reset/rates/controls/extremes passed\n";
}
int main(){devicesAndBias();transients();recoveryReference();controls();historyAndSafety();std::cout<<"Muff checks="<<testChecks<<" passed\n";}
