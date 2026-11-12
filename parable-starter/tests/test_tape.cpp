#include "parable_tape.h"
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
using namespace parable::dsp;
namespace g=activation_guard;
static std::vector<std::string> fields(const std::string& line) {
    std::stringstream in(line);std::vector<std::string> out;std::string s;
    while(std::getline(in,s,','))out.push_back(s);
    return out;
}
static void reference() {
    std::ifstream in(std::string(TAPE_FIXTURES)+"/reference.csv");CHECK(bool(in));std::string line,current;
    std::getline(in,line);Tape tape;std::vector<float> memory(Tape::kMaximumCells);size_t sample=0,values=0;double peak=0;
    TapeConfig c;c.loopSeconds=.1f;c.delaySeconds=.02f;c.mix=1;
    while(std::getline(in,line)) {
        const auto v=fields(line);CHECK(v.size()==12);const auto key=v[0]+v[1]+v[2];const auto rate=std::stof(v[0]);
        if(key!=current) {current=key;sample=0;c.eraseRetention=std::stof(v[1]);c.feedback=std::stof(v[2]);CHECK(tape.prepare(rate,c,memory.data(),memory.size()));}
        const auto target=std::stoul(v[3]);const auto n=tape.cells();
        for(;sample<=target;++sample) {
            const auto phase=sample%n;
            const float x=sample<n ? (phase<n/10?.2f:phase<n/5?-.1f:0) : sample<2*n ? (phase<n/8?-.15f:phase<n/4?.05f:0):0;
            g::begin();const auto y=tape.process(x);g::end();CHECK(g::clean());CHECK(std::isfinite(y)&&std::abs(y)<=1);
        }
        const auto p=tape.probe();const float actual[]={p.eraseBefore,p.eraseAfter,p.writeBefore,p.recordValue,p.written,p.playMagnetization,p.replay};
        // Decimal tails can be smaller than binary64; strtod rounds underflow
        // to zero instead of throwing. Absolute numerical gate still applies.
        for(size_t i=0;i<7;++i) {const double expected=std::strtod(v[i+5].c_str(),nullptr);CHECK(std::isfinite(expected));peak=std::max(peak,std::abs(double(actual[i])-expected));++values;}
        CHECK(p.writeCell==target%n);CHECK(p.eraseCell==(target+tape.eraseCells())%n);
        CHECK(p.playCell==(target+n-tape.delayCells())%n);CHECK(p.pass==target/n);
    }
    CHECK(values>10000);CHECK(peak<2e-6);
    std::cout<<"Decimal independent-reference values="<<values<<" peak_error="<<peak<<'\n';
}
static void ghost() {
    const unsigned rate=48000;TapeConfig c;c.loopSeconds=.1f;c.delaySeconds=.02f;c.mix=1;c.feedback=0;
    const auto n=Tape::requiredCells(rate,c);std::vector<float> mem1(n),mem2(n);Tape ab,b;
    for(float r:{0.f,Tape::kFaultRetention,.95f}) {
        c.eraseRetention=r;CHECK(ab.prepare(rate,c,mem1.data(),n));CHECK(b.prepare(rate,c,mem2.data(),n));
        double first=0;std::vector<double> sums(9);
        for(size_t i=0;i<9*n+ab.delayCells();++i) {
            const auto phase=i%n,turn=i/n;const float a=phase<512?.15f*std::sin(float(phase)*.13f):0;
            const float different=phase<512?.15f*std::sin(float(phase)*.37f):0;
            const float x=turn==0?a:different;
            g::begin();const float ya=ab.process(x),yb=b.process(turn==0?0:different);g::end();CHECK(g::clean());
            if(i>=ab.delayCells()) {const auto age=(i-ab.delayCells())/n;if(age<9)sums[age]+=double(ya-yb)*(ya-yb);}
        }
        first=std::sqrt(sums[0]);CHECK(first>.1);
        for(size_t age=1;age<9;++age) {
            const double gain=std::sqrt(sums[age])/first,expected=std::pow(double((1-Tape::kWriteFraction)*r),double(age));
            CHECK(std::abs(gain-expected)<2e-6);if(r==0)CHECK(gain<1e-7);
        }
        if(r==Tape::kFaultRetention)std::cout<<"Feedback-zero A under DIFFERENT B: measured gain="<<std::sqrt(sums[1])/first<<"; subsequent turns ratio=.34; healthy <=1e-7\n";
    }
    // Healthy feedback repeats appear after D, and disappear with feedback zero:
    // they are not a once-per-loop magnetic residual.
    for(float f:{0.f,.8f}) {
        c.eraseRetention=0;c.feedback=f;CHECK(ab.prepare(rate,c,mem1.data(),n));
        double repeat=0;for(size_t i=0;i<3*ab.delayCells();++i){const auto y=ab.process(i==0?.1f:0);if(i>=2*ab.delayCells())repeat+=y*y;}
        CHECK(f==0?repeat<1e-20:repeat>1e-8);
    }
}
static void safety() {
    g::selfTest();Tape t,other;std::vector<float> a(Tape::kMaximumCells+2,123),b(Tape::kMaximumCells);TapeConfig c;
    CHECK(t.process(.2f)==0);
    for(float rate:{44100.f,48000.f,96000.f})for(float seconds:{.1f,2.f}) {
        c.loopSeconds=seconds;c.delaySeconds=seconds==.1f?.02f:1.98f;c.eraseRetention=.95f;c.feedback=.8f;c.drive=8;
        const auto n=Tape::requiredCells(rate,c);CHECK(n==uint32_t(rate*seconds+.5f));
        CHECK(t.prepare(rate,c,a.data()+1,n));CHECK(t.cells()==n);CHECK(n*sizeof(float)<=768000);
        const float edges[]={0,NAN,INFINITY,-INFINITY,std::numeric_limits<float>::max(),-std::numeric_limits<float>::max()};
        for(size_t i=0;i<2*n+32;++i){g::begin();const float y=t.process(i<32?edges[i%6]:i%2?1.f:-1.f);g::end();CHECK(g::clean());CHECK(std::isfinite(y)&&std::abs(y)<=1);CHECK(std::abs(t.probe().written)<=1);}
        CHECK(a.front()==123&&a[n+1]==123);t.stop();const auto old=t.probe();CHECK(t.process(1)==0);CHECK(t.probe().writeCell==old.writeCell);
        t.reset();for(unsigned i=0;i<100;++i)CHECK(t.process(0)==0);
        std::fill(a.begin(),a.end(),123);
    }
    c={};CHECK(t.prepare(48000,c,a.data()+1,a.size()-1));CHECK(other.prepare(48000,c,b.data(),b.size()));
    for(unsigned i=0;i<100;++i)CHECK(t.process(.1f)==other.process(.1f));
    CHECK(!t.prepare(48000,c,nullptr,999999));CHECK(!t.prepare(48000,c,a.data()+1,1));
    for(float invalid:{NAN,INFINITY,-1.f,0.f,31999.f,96001.f})CHECK(!t.prepare(invalid,c,a.data()+1,a.size()-1));
    for(unsigned field=0;field<8;++field)for(float value:{NAN,INFINITY,-1.f,100000.f}) {
        auto bad=c;float* p[]={&bad.loopSeconds,&bad.delaySeconds,&bad.eraseRetention,&bad.feedback,&bad.drive,&bad.recordHz,&bad.replayHz,&bad.mix};*p[field]=value;
        CHECK(!t.prepare(48000,bad,a.data()+1,a.size()-1));
    }
    auto collision=c;collision.loopSeconds=.1f;collision.delaySeconds=.09f;CHECK(!Tape::requiredCells(48000,collision));
    for(unsigned i=0;i<5000;++i)CHECK(t.process(.1f)==other.process(.1f)); // failed prepares left history intact.
    // Saturated endpoint histories must actually decay with zero input, without
    // reset, even at the maximum supported retention/regenerative feedback.
    c.loopSeconds=.1f;c.delaySeconds=.02f;c.eraseRetention=.95f;c.feedback=.8f;c.drive=8;c.mix=1;
    CHECK(t.prepare(48000,c,a.data()+1,a.size()-1));
    for(size_t i=0;i<t.cells();++i)t.process(1);
    float tailPeak=0;
    for(size_t i=0;i<96*t.cells();++i) {const auto y=t.process(0);CHECK(std::isfinite(y)&&std::abs(y)<=1);if(i>=95*t.cells())tailPeak=std::max(tailPeak,std::abs(y));}
    CHECK(tailPeak<1e-6f);
    for(float rate:{44100.f,48000.f,96000.f})for(bool high:{false,true}) {
        c.loopSeconds=.1f;c.delaySeconds=.02f;c.drive=high?8.f:.25f;
        c.eraseRetention=high?.95f:0;c.feedback=high?.8f:0;c.mix=high?1.f:0;
        c.recordHz=c.replayHz=high?10000.f:200.f;
        CHECK(t.prepare(rate,c,a.data()+1,a.size()-1));
        for(size_t i=0;i<3*t.cells();++i){const float x=i%2?.9f:-.9f;const auto y=t.process(x);CHECK(std::isfinite(y)&&std::abs(y)<=1);if(!high)CHECK(y==x);}
    }
    CHECK(sizeof(Tape)<=192);
    std::cout<<"Tape layout="<<sizeof(Tape)<<" max_medium=768000 bytes; callback guards clean\n";
}
int main(){reference();ghost();safety();std::cout<<"Tape checks="<<testChecks<<" passed\n";}
