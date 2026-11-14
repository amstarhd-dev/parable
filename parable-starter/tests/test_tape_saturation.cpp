#include "parable_tape_saturation.h"
#include "tape_saturation_material.h"
#include "tape_saturation_stimulus.h"
#include "activation_guard.h"
#include "test_check.h"
#include <complex>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>
using parable::dsp::TapeSaturation;
namespace material=parable::dsp::tape_saturation_detail;
namespace guard=activation_guard;
namespace {
std::filesystem::path evidence;
std::ofstream output(const char* name){if(evidence.empty())return {};std::ofstream f(evidence/name);CHECK(bool(f));f<<std::setprecision(16);return f;}
void cdfAndLoops() {
    std::array<float,129> cdf{},slope{};material::prepareCDF(cdf,slope);
    double previous=-1,error=0;for(int n=-800000;n<=800000;++n){const double x=n/100000.,y=material::gaussian(x,cdf,slope);
        CHECK(y>=previous-1e-15&&std::abs(y)<=1);previous=y;error=std::max(error,std::abs(y-std::erf(x/std::sqrt(2.))));}
    CHECK(error<3e-7);
    // Independent explicit MIT-directional relays, integrated with positive
    // Gaussian cell masses. No condensed-boundary implementation used here.
    constexpr unsigned cells=4001;constexpr double width=16*12000./cells;
    std::array<double,cells> center{},mass{};std::array<std::array<int,cells>,8> relay{};
    for(unsigned j=0;j<cells;++j){center[j]=-96000+(j+.5)*width;const double lo=(center[j]-width/2)/(12000*std::sqrt(2.)),hi=(center[j]+width/2)/(12000*std::sqrt(2.));mass[j]=.5*(std::erf(hi)-std::erf(lo));for(auto& row:relay)row[j]=center[j]<0?1:-1;}
    std::array<double,8> p{},w{};double norm=0;for(unsigned i=0;i<8;++i){w[i]=std::exp(-.5*std::pow((material::radius[i]-25500)/6000,2));norm+=w[i];}for(auto& v:w)v/=norm;
    auto f=output("loops.csv");if(f)f<<"H_A_per_m,M_condensed,M_explicit,normalized_work\n";
    double lastH=0,lastM=0,work=0,maxError=0,positiveRemanence=0,negativeRemanence=0;
    const auto move=[&](double target){const unsigned steps=unsigned(std::abs(target-lastH)/250)+1;const double start=lastH;
        for(unsigned k=1;k<=steps;++k){const double h=start+(target-start)*k/steps;double condensed=0,explicitM=0;
            for(unsigned i=0;i<8;++i){p[i]=material::advance(p[i],h,material::radius[i]);condensed+=w[i]*material::gaussian(p[i]/12000,cdf,slope);
                for(unsigned j=0;j<cells;++j){if(h>lastH&&h>center[j]+material::radius[i])relay[i][j]=1;else if(h<lastH&&h<center[j]-material::radius[i])relay[i][j]=-1;explicitM+=w[i]*mass[j]*relay[i][j];}}
            maxError=std::max(maxError,std::abs(explicitM-condensed));work+=.5*(lastH+h)*(condensed-lastM);
            if(f)f<<h<<','<<condensed<<','<<explicitM<<','<<work<<'\n';
            lastH=h;lastM=condensed;
        }};
    move(120000);CHECK(lastM>.9999);move(0);positiveRemanence=lastM;CHECK(lastM>.8);const auto held=p;move(1000);move(0);CHECK(p==held);
    move(-120000);CHECK(lastM<-.9999);move(0);negativeRemanence=lastM;CHECK(std::abs(positiveRemanence+negativeRemanence)<1e-7);
    move(40000);const auto outer=p;move(10000);const auto inner=p;move(30000);move(10000);CHECK(p==inner);move(40000);CHECK(p==outer);
    move(120000);const auto closedStart=p;const double startWork=work;move(-120000);move(120000);
    const double closedWork=work-startWork;CHECK(p==closedStart&&closedWork>80000&&maxError<.002);
    // Closed major loop work is dissipative for each independently resolved row;
    // a static tanh cannot preserve nonzero remanence or these hold/minor loops.
    std::cout<<"CDF peak="<<error<<" explicit-relay peak="<<maxError<<" remanence="<<positiveRemanence<<" closed_work="<<closedWork<<'\n';
}
void reference() {
    std::ifstream f(std::string(TAPE_SATURATION_FIXTURES)+"/material.csv");CHECK(bool(f));std::string line,key;std::getline(f,line);TapeSaturation m;unsigned next=0,count=0,groups=0,rows=0;double sum=0,peak=0,refSum=0,refPeak=0,refinement=0;bool harsh=false;
    auto out=output("accuracy.csv");if(out)out<<"condition,count,RMS_error,peak_error,reference_RMS,reference_peak\n";
    const auto finish=[&](){if(!count)return;const double rms=std::sqrt(sum/count),rr=std::sqrt(refSum/count);CHECK(rms<(harsh?.005:.001)+(harsh?.05:.03)*rr);CHECK(peak<(harsh?.02:.003)+.05*refPeak);if(out)out<<key<<','<<count<<','<<rms<<','<<peak<<','<<rr<<','<<refPeak<<'\n';};
    while(std::getline(f,line)){std::stringstream s(line);std::vector<std::string> r;std::string word;while(std::getline(s,word,','))r.push_back(word);CHECK(r.size()==7);
        const auto group=r[0]+"_"+r[1]+"_"+r[2]+"_"+r[3];const double amplitude=std::stod(r[1]);const unsigned rate=unsigned(std::stoul(r[2])),target=unsigned(std::stoul(r[4]));
        if(group!=key){finish();key=group;++groups;CHECK(m.prepare(float(rate),r[3]=="1"));next=count=0;sum=peak=refSum=refPeak=0;harsh=amplitude>=.8;}
        float value=0;for(;next<=target;++next){const float input=float(tapelab::stimulus(r[0],double(next)/(4*rate),amplitude));guard::begin();value=m.materialReference(input);guard::end();CHECK(guard::clean());}
        const double expected=std::stod(r[5]),fine=std::stod(r[6]),e=std::abs(value-expected);sum+=e*e;peak=std::max(peak,e);refSum+=expected*expected;refPeak=std::max(refPeak,std::abs(expected));refinement=std::max(refinement,std::abs(expected-fine));++count;
        CHECK(!m.failures()&&!m.invalidInputs()&&!m.inputClamps()&&!m.outputClamps());++rows;
    }finish();CHECK(groups==108&&rows==4428);CHECK(refinement<.002);std::cout<<"Long-double reference refinement peak="<<refinement<<'\n';
}
void playback() {
    auto f=output("playback.csv");if(f)f<<"rate_hz,hz,native_magnitude,analytic_magnitude,error_dB\n";double worst=0;
    for(unsigned rate:{44100u,48000u,96000u}){TapeSaturation m;CHECK(m.prepare(float(rate)));double sum=0;for(float c:m.playbackKernel())sum+=c;CHECK(std::abs(sum-1)<1e-6);
        for(double hz:{100.,250.,500.,1000.,2000.,4000.,8000.,12000.,16000.}){std::complex<double> result=0;unsigned n=0;for(float c:m.playbackKernel())result+=double(c)*std::polar(1.,-2*tapelab::pi*hz*n++/rate);
            const double k=2*tapelab::pi*hz/.381,x=k*3e-6/2,u=k*16e-6,expected=std::sin(x)/x*std::exp(-k*.3e-6)*(-std::expm1(-u)/u),err=20*std::log10(std::abs(result)/expected);worst=std::max(worst,std::abs(err));CHECK(std::abs(err)<.5);if(f)f<<rate<<','<<hz<<','<<std::abs(result)<<','<<expected<<','<<err<<'\n';}
    }std::cout<<"Playback magnitude worst_dB="<<worst<<'\n';
}
void lifecycle() {
    TapeSaturation unprepared;CHECK(unprepared.process(1)==0&&unprepared.materialReference(1)==0);CHECK(!unprepared.prepare(32000));
    for(unsigned rate:{44100u,48000u,96000u})for(bool weak:{false,true}){TapeSaturation a,b;CHECK(a.prepare(float(rate),weak)&&b.prepare(float(rate),weak));CHECK(a.biasAmps()==(weak?.006:.012));
        CHECK(!a.prepare(std::numeric_limits<float>::quiet_NaN())&&!a.prepare(std::numeric_limits<float>::infinity())&&!a.prepare(96001));
        for(unsigned n=0;n<rate/50;++n){const float x=n<rate/200?0:float(tapelab::stimulus("two",double(n)/rate,.8));guard::begin();const float y=a.process(x);guard::end();CHECK(guard::clean()&&y==b.process(x)&&std::isfinite(y)&&std::abs(y)<1);if(n<rate/200)CHECK(y==0);}
        CHECK(!a.failures()&&!a.inputClamps()&&!a.outputClamps());const auto old=a.probe().remanence;
        CHECK(a.process(std::numeric_limits<float>::quiet_NaN())==0&&a.process(std::numeric_limits<float>::infinity())==0&&a.invalidInputs()==2&&a.probe().remanence==old);
        a.reset();b.reset();for(unsigned n=0;n<rate/500;++n){const float x=n==0?2.f:0;CHECK(a.process(x)==b.process(x));}CHECK(!a.failures());
        a.process(std::numeric_limits<float>::max());CHECK(a.inputClamps()==1&&!a.failures());a.stop();CHECK(a.process(1)==0);a.reset();CHECK(a.process(0)==0);
        parable::dsp::TapeParcelTrace trace;CHECK(a.traceParcel(0,trace)&&trace.count>40&&trace.count<=48);CHECK(trace.points[trace.count-1].field_A_per_m==0);CHECK(!a.traceParcel(8,trace));
        a.reset();for(unsigned n=0;n<256;++n)CHECK(std::abs(a.materialReference(n<128?64.f:-64.f))<.94834f);CHECK(!a.inputClamps()&&!a.failures());
        a.reset();b.reset();for(unsigned n=0;n<rate/500;++n){const float x=float(tapelab::stimulus("two",double(n)/rate,.8));CHECK(std::abs(a.process(x)+b.process(-x))<1e-7);}
    }
}
}
int main(int argc,char** argv)try {
    if(argc==3&&std::string(argv[1])=="--evidence"){evidence=argv[2];std::filesystem::create_directories(evidence);}else CHECK(argc==1);
    cdfAndLoops();reference();playback();lifecycle();std::cout<<"SF012 material, reference, passive history, playback, guarded audio/lifecycle passed; state_bytes="<<sizeof(TapeSaturation)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
