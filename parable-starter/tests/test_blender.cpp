#include "parable_blender.h"
#include "blender_device.h"
#include "activation_guard.h"
#include "test_check.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
using parable::dsp::Blender;
using parable::dsp::BlenderConfig;
using parable::dsp::BlenderRate;
namespace dev=parable::dsp::blender_detail;
namespace guard=activation_guard;
using Row=std::vector<std::string>;
static std::vector<Row> csv(const char* file) {
    std::ifstream in(std::string(BLENDER_FIXTURES)+"/"+file);CHECK(bool(in));std::string line;std::getline(in,line);std::vector<Row> rows;
    while(std::getline(in,line)){Row r;std::stringstream s(line);std::string v;while(std::getline(s,v,','))r.push_back(v);rows.push_back(r);}return rows;
}
static double val(const Row& r,std::size_t n){return std::stod(r.at(n));}
static BlenderConfig config(const Row& r,std::size_t n,bool effect=false) {
    BlenderConfig c;c.d1Open=val(r,n)!=0;c.sustain=float(val(r,n+1));c.tone=float(val(r,n+2));c.blend=float(val(r,n+3));c.level=float(val(r,n+4));c.boost=val(r,n+5)!=0;
    c.supplyVolts=float(val(r,n+6));c.temperatureC=float(val(r,n+7));c.beta=float(val(r,n+8));c.sourceOhms=float(val(r,n+9));c.loadOhms=float(val(r,n+10));c.junctionScale=float(val(r,n+11));if(effect)c.effect=val(r,n+12)!=0;return c;
}
static double observed(const Blender& m,std::size_t n){return n<31?m.nodes()[n]:n<46?m.transistorAmps((n-31)/3,(n-31)%3):m.diodeAmps(n-46);}
static void devicesAndBias() {
    for(double theta:{1/(dev::kOverQ*300),32.})for(double v:{-15.,-.6,-.1,0.,.1,.6}){
        const auto d=dev::junction(v,theta,theta==32?560e-9:1e-14,dev::transit);CHECK(d.valid);
        const auto lo=dev::junction(v-1e-7,theta,theta==32?560e-9:1e-14,dev::transit),hi=dev::junction(v+1e-7,theta,theta==32?560e-9:1e-14,dev::transit);
        CHECK(std::abs((hi.i-lo.i)/2e-7-d.g)<=1e-10+1e-4*std::abs(d.g));CHECK(std::abs((hi.q-lo.q)/2e-7-d.c)<=1e-18+1e-4*std::abs(d.c));
    }
    CHECK(!dev::junction(std::numeric_limits<double>::quiet_NaN(),32,1e-6).valid);
    double peakV=0,peakI=0;std::size_t points=0;
    for(const auto& r:csv("bias.csv")){
        Blender m;CHECK(m.prepare(48000,config(r,0)));CHECK(r.size()==62);
        for(std::size_t k=0;k<50;++k){const double error=std::abs(observed(m,k)-val(r,12+k));if(k<31)peakV=std::max(peakV,error);else peakI=std::max(peakI,error);++points;}
        if(val(r,0)){CHECK(m.diodeBranchAmps(0)==0);CHECK(m.biasNodes()[Blender::C4]<1.5);}else CHECK(m.diodeBranchAmps(0)>0);
    }
    std::cout<<"SPICE DC values="<<points<<" peak_V="<<peakV<<" peak_A="<<peakI<<std::endl;CHECK(peakV<=1e-5);CHECK(peakI<=2e-9);
}
static uint32_t u32(std::istream& in){unsigned char b[4]{};in.read(reinterpret_cast<char*>(b),4);CHECK(bool(in));return uint32_t(b[0])|uint32_t(b[1])<<8|uint32_t(b[2])<<16|uint32_t(b[3])<<24;}
static float f32(std::istream& in){static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559,"fixture IEEE754");const uint32_t bits=u32(in);float f=0;std::memcpy(&f,&bits,4);CHECK(std::isfinite(f));return f;}
static float stimulus(const Row& c,unsigned frame) {
    const double rate=val(c,5),t=frame/rate,hz=val(c,2),a=val(c,3);double x=a*std::sin(6.2831853071795864769*hz*t);
    if(c[1]=="chord")x=a/3*(std::sin(6.2831853071795864769*110*t)+std::sin(6.2831853071795864769*138.59*t)+std::sin(6.2831853071795864769*164.81*t));
    x*=1-std::exp(-t/.002);if(c[1]=="pluck")x*=std::exp(-t/.12);return float(x);
}
static void transients() {
    const auto cases=csv("cases.csv");CHECK(cases.size()==48);std::ifstream in(std::string(BLENDER_FIXTURES)+"/transient.f32",std::ios::binary);CHECK(bool(in));char magic[8]{};in.read(magic,8);CHECK(std::memcmp(magic,"SF016F32",8)==0);
    const auto rows=u32(in);CHECK(rows>30000&&rows<150000);CHECK(u32(in)==50);
    struct Sum {double square=0,reference=0,referenceSquare=0;};std::array<Sum,50> sums{};std::array<double,50> peak{};
    Blender m;unsigned group=999,next=0,count=0,misses=0;double worstRatio=0;std::size_t groups=0;
    const auto finish=[&]{if(!count)return;for(std::size_t k=0;k<50;++k){const auto& s=sums[k];const double rms=std::sqrt(s.square/count),ac=std::sqrt(std::max(0.,s.referenceSquare/count-std::pow(s.reference/count,2))),limit=.03*ac+(k==Blender::Jack?50e-6:k<31?.0005:.1e-6);worstRatio=std::max(worstRatio,rms/limit);if(rms>limit){++misses;std::cout<<"MISS case="<<group<<' '<<cases[group][1]<<" field="<<k<<" rms="<<rms<<" limit="<<limit<<" peak="<<peak[k]<<'\n';}}};
    for(uint32_t row=0;row<rows;++row){
        const auto id=u32(in),frame=u32(in);CHECK(id<cases.size());std::array<double,50> expected{};for(auto& v:expected)v=f32(in);
        if(id!=group){finish();group=id;next=count=0;sums={};peak={};CHECK(unsigned(val(cases[id],0))==id);CHECK(m.prepare(float(val(cases[id],5)),config(cases[id],6,true),BlenderRate::Direct));++groups;}
        CHECK(frame>=next||frame==0);
        for(;next<=frame;++next){const float x=stimulus(cases[id],next);guard::begin();const auto y=m.process(x);guard::end();CHECK(guard::clean());CHECK(std::isfinite(y)&&std::abs(y)<=1);}
        for(std::size_t k=0;k<50;++k){const double actual=observed(m,k),e=actual-expected[k];sums[k].square+=e*e;sums[k].reference+=expected[k];sums[k].referenceSquare+=expected[k]*expected[k];peak[k]=std::max(peak[k],std::abs(e));}
        ++count;CHECK(!m.failures()&&!m.inputClamps()&&!m.outputClamps()&&!m.invalidInputs());CHECK(m.maximumEvaluations()<=Blender::kUpdates+1);
    }
    finish();CHECK(in.peek()==std::char_traits<char>::eof());std::cout<<"SPICE full-history/dense-cycle groups="<<groups<<" rows="<<rows<<" gate_misses="<<misses<<" worst_error/limit="<<worstRatio<<std::endl;CHECK(groups==48);CHECK(misses==0);
}
static void safetyAndHistory() {
    const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    Blender unprepared;CHECK(unprepared.process(.1f)==0);
    for(float rate:{44100.f,48000.f,96000.f})for(bool fault:{false,true})for(auto mode:{BlenderRate::Direct,BlenderRate::X2,BlenderRate::X4,BlenderRate::X8}){
        BlenderConfig c;c.d1Open=fault;Blender a,b;CHECK(a.prepare(rate,c,mode)&&b.prepare(rate,c,mode));
        for(unsigned n=0;n<512;++n){const float x=float(.1*std::sin(.03*n));guard::begin();const auto y=a.process(x);guard::end();CHECK(y==b.process(x));CHECK(guard::clean());}
        for(float x:{nan,inf,-inf}){guard::begin();const auto y=a.process(x);guard::end();CHECK(y==b.process(0));}CHECK(a.invalidInputs()==3);
        // Invalid controls/rates must preserve evolving signal/history exactly.
        for(float bad:{nan,inf,-1.f,1.01f})for(unsigned field=0;field<4;++field){auto v=c;float* fields[]={&v.sustain,&v.tone,&v.blend,&v.level};*fields[field]=bad;CHECK(!a.prepare(rate,v,mode));}
        for(float bad:{nan,inf,-inf})for(unsigned field=0;field<6;++field){auto v=c;float* fields[]={&v.supplyVolts,&v.temperatureC,&v.beta,&v.sourceOhms,&v.loadOhms,&v.junctionScale};*fields[field]=bad;CHECK(!a.prepare(rate,v,mode));}
        for(float bad:{0.f,-48000.f,nan,inf,44099.f,3072001.f}){CHECK(!a.prepare(bad,c,BlenderRate::Direct));}
        CHECK(!a.prepare(rate,c,BlenderRate(3)));
        auto invalid=c;invalid.junctionScale=0;CHECK(!a.prepare(rate,invalid,mode));invalid=c;invalid.beta=100;CHECK(!a.prepare(rate,invalid,mode));
        for(unsigned n=0;n<128;++n){const float x=float(.04*std::cos(n*.11));CHECK(a.process(x)==b.process(x));}
        a.stop();CHECK(a.process(.3f)==0);a.reset();b.reset();
        std::vector<float> first;for(unsigned n=0;n<1024;++n)first.push_back(a.process(n<300?.1f:0.f));a.reset();for(unsigned n=0;n<1024;++n)CHECK(a.process(n<300?.1f:0.f)==first[n]);
        CHECK(a.failures()==0&&a.outputClamps()==0);a.reset();for(unsigned n=0;n<512;++n)CHECK(std::abs(a.process(0))<=1e-8);
        for(float x:{std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),2.f,-2.f,std::numeric_limits<float>::denorm_min()}){guard::begin();const auto y=a.process(x);guard::end();CHECK(std::isfinite(y)&&std::abs(y)<=1);CHECK(guard::clean());}CHECK(a.inputClamps()==4);
        for(unsigned n=0;n<2048;++n){guard::begin();const auto y=a.process(0);guard::end();CHECK(std::isfinite(y)&&std::abs(y)<=1);CHECK(guard::clean());}a.reset();CHECK(a.process(0)==0);
    }
    // Real electrical endpoints, switches, supply/temperature/source/load extremes.
    for(bool fault:{false,true})for(bool boost:{false,true})for(float end:{0.f,1.f}){BlenderConfig c;c.d1Open=fault;c.boost=boost;c.sustain=c.tone=c.blend=c.level=end;c.supplyVolts=end?10:7.5f;c.temperatureC=end?40:5;c.sourceOhms=end?10000:100;c.loadOhms=end?2000000:100000;Blender m;CHECK(m.prepare(48000,c));for(unsigned n=0;n<1024;++n){guard::begin();const auto y=m.process(float(.05*std::sin(n*.03)));guard::end();CHECK(std::isfinite(y));CHECK(guard::clean());}CHECK(!m.failures()&&!m.outputClamps());}
}
int main(int argc,char** argv){CHECK(argc==1||(argc==2&&std::string(argv[1])=="--safety-only"));guard::selfTest();devicesAndBias();if(argc==1)transients();safetyAndHistory();std::cout<<"SF016 state/safety checks passed; circuit reference "<<(argc==1?"checked":"not requested")<<'\n';}
