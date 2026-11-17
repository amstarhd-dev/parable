#include "parable_chamber.h"
#include "activation_guard.h"
#include "test_check.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using parable::soundforge::Chamber;
constexpr double pi=3.14159265358979323846;
static void curve(){std::ifstream f(std::string(CHAMBER_FIXTURES)+"/return-amplifier.csv");CHECK(bool(f));std::string row;std::getline(f,row);unsigned count=0;double peak=0;
 while(std::getline(f,row)){std::replace(row.begin(),row.end(),',',' ');std::istringstream s(row);double x,g,y;CHECK(bool(s>>x>>g>>y));peak=std::max(peak,std::abs(Chamber::returnAmplifier(float(x),float(g))-y));++count;}
 CHECK(count==75&&peak<1.e-7);CHECK(Chamber::returnAmplifier(.006f,64)==0);CHECK(std::abs(Chamber::returnAmplifier(.00700001f,24))<1.e-6);std::cout<<"Independent amplifier law75 points peak_error="<<peak<<'\n';}
static float excitation(unsigned n,float rate,float amplitude){return n<rate*.5f?float(amplitude*(std::sin(2*pi*220*n/rate)+.5*std::sin(2*pi*553*n/rate))/1.5*std::exp(-double(n)/8000.)):0;}
struct Probe{double late=0,early=0,returnPeak=0,outputPeak=0;unsigned clamps=0;};
static Probe run(float rate,float gain,float amplitude,bool disconnect=false,unsigned seconds=14){Chamber e;CHECK(e.prepare(rate,gain,1));float input[64]{},output[64]{};Probe p;bool finite=true;
 activation_guard::begin();for(unsigned base=0;base<unsigned(rate*seconds);base+=64){if(disconnect&&base>=unsigned(rate*8))e.setReturnGain(0);for(unsigned j=0;j<64;++j)input[j]=excitation(base+j,rate,amplitude);finite&=e.process(input,output);
 for(unsigned j=0;j<64;++j){const auto n=base+j;finite&=std::isfinite(output[j]);p.outputPeak=std::max(p.outputPeak,std::abs(double(output[j])));p.returnPeak=std::max(p.returnPeak,std::abs(double(e.returnSignal()[j])));if(n>rate*(seconds-2))p.late+=double(output[j])*output[j];if(n>rate*.1&&n<rate*.5)p.early+=double(output[j])*output[j];}}
 activation_guard::end();CHECK(finite&&activation_guard::clean());p.late=std::sqrt(p.late/(rate*2));p.clamps=e.outputClamps();CHECK(p.clamps==0&&p.outputPeak<.98&&p.returnPeak<=.150001);return p;}
static void dynamics(){
 for(float rate:{44100.f,48000.f,96000.f}){const auto weak=run(rate,24,.005f),healthy=run(rate,.02f,.15f),strong=run(rate,24,.15f),zero=run(rate,24,0);
  CHECK(weak.returnPeak==0&&zero.outputPeak==0);CHECK(healthy.late<1.e-5&&strong.late>.005);CHECK(strong.late>healthy.late*1000);std::cout<<"rate="<<rate<<" weak_return="<<weak.returnPeak<<" zero="<<zero.outputPeak<<" healthy_late="<<healthy.late<<" fault_late="<<strong.late<<" fault_peak="<<strong.outputPeak<<'\n';}
 const auto recovered=run(48000,24,.15f,true);CHECK(recovered.late<.001);std::cout<<"Disconnect gain at8s,12-14s recovery RMS="<<recovered.late<<'\n';
 const auto longRun=run(48000,24,.15f,false,60);CHECK(longRun.late>.005&&longRun.clamps==0);std::cout<<"60s unforced fault remains finite/bounded; final2s RMS="<<longRun.late<<" peak="<<longRun.outputPeak<<'\n';
}
static void causality(){Chamber e;CHECK(e.prepare(48000,24,1));std::array<float,64> previous{},input{},output{};double hp=0,lp=0,prev=0,peak=0;const double a=std::exp(-2*pi*200/48000),b=std::exp(-2*pi*4500/48000);bool good=true;
 activation_guard::begin();for(unsigned base=0;base<48000;++base){if(base%64)continue;for(unsigned j=0;j<64;++j)input[j]=excitation(base+j,48000,.15f);good&=e.process(input.data(),output.data());
 for(unsigned j=0;j<64;++j){peak=std::max(peak,std::abs(double(e.speaker()[j])-(input[j]+previous[j])));peak=std::max(peak,std::abs(double(e.returnSignal()[j])-previous[j]));const double mic=e.microphone()[j];hp=a*(hp+mic-prev);prev=mic;lp+=(1-b)*(hp-lp);const double z=std::copysign(std::max(std::abs(lp)-.007,0.),lp)*24/.15;previous[j]=float(.15*std::tanh(z));}}
 activation_guard::end();CHECK(good&&activation_guard::clean()&&peak<5.e-6);std::cout<<"Independent double mic-filter/amp/64sample excitation wiring peak_error="<<peak<<'\n';
}
static void safety(){Chamber e;float input[64]{},out[64]{};CHECK(!e.process(input,out));CHECK(e.prepare(48000,0,1));std::vector<float> first(8192);double energy=0;bool repeat=true;
 for(unsigned n=0;n<8192;n+=64){for(unsigned j=0;j<64;++j)input[j]=excitation(n+j,48000,.15f);repeat&=e.process(input,out);std::copy_n(out,64,first.data()+n);for(float x:out)energy+=double(x)*x;}
 CHECK(repeat&&energy>1.e-4);CHECK(e.prepare(48000,0,1));for(unsigned n=0;n<8192;n+=64){for(unsigned j=0;j<64;++j)input[j]=excitation(n+j,48000,.15f);repeat&=e.process(input,out);repeat&=std::equal(out,out+64,first.data()+n);}CHECK(repeat);
 Chamber reference;CHECK(reference.prepare(48000,0,1));for(unsigned n=0;n<8192;n+=64){for(unsigned j=0;j<64;++j)input[j]=excitation(n+j,48000,.15f);reference.process(input,out);}
 CHECK(!e.prepare(32000)&&!e.prepare(NAN)&&!e.prepare(48000,-1)&&!e.setReturnGain(INFINITY));input[1]=NAN;CHECK(!e.process(input,out));input[1]=0;float expected[64];CHECK(e.process(input,out)&&reference.process(input,expected));CHECK(std::equal(out,out+64,expected));
 CHECK(!e.prepare(48000,0,1,{-1,.25f,6500}));CHECK(!e.prepare(48000,0,1,{0,NAN,6500}));CHECK(!e.prepare(48000,0,1,{0,.25f,1999}));
 CHECK(e.prepare(48000,0,1,{40,.5f,2000}));std::fill_n(input,64,0.f);input[0]=.1f;bool predelay=true;double after=0;for(unsigned n=0;n<4096;n+=64){e.process(input,out);std::fill_n(input,64,0.f);for(unsigned j=0;j<64;++j){if(n+j<1920)predelay&=out[j]==0;else after+=double(out[j])*out[j];}}CHECK(predelay&&after>0);
 CHECK(e.prepare(48000,0,1));std::fill_n(input,64,0.f);bool silent=true;for(unsigned i=0;i<100;++i){silent&=e.process(input,out);for(float x:out)silent&=x==0;}CHECK(silent);
 // Stress the common numerical input/output containment; no musical assertion.
 std::fill_n(input,64,1.e30f);bool finite=true;activation_guard::begin();for(unsigned i=0;i<200;++i){finite&=e.process(input,out);for(float x:out)finite&=std::isfinite(x)&&std::abs(x)<=.98f;}activation_guard::end();CHECK(finite&&activation_guard::clean());
 std::cout<<"Finite/rejected input, stopped reprepare repeatability, silence and callback guards passed; core="<<Chamber::coreBytes()<<" wrapper="<<sizeof(Chamber)<<'\n';
}
int main(int argc,char**argv){
 if(argc==3){auto source=lab::read(argv[1]);source.samples.resize(48000*8);const double gain=.15/soundforge::stats(source.samples).peak;for(auto&x:source.samples)x=float(x*gain);source.samples.resize(48000*14,0);auto expected=lab::read(argv[2]);Chamber e;CHECK(e.prepare(48000,0,1));lab::Wave actual{48000,std::vector<float>(source.samples.size())};float output[64];for(unsigned n=0;n<source.samples.size();n+=64){e.process(source.samples.data()+n,output);std::copy_n(e.microphone(),64,actual.samples.data()+n);}CHECK(lab::encode(actual)==lab::encode(expected));std::cout<<"Original pinned core vs initializer-fixed selected core: actual take03 mic WAV BIT IDENTICAL with return disconnected\n";return 0;}
 activation_guard::selfTest();curve();causality();dynamics();safety();std::cout<<"Focused checks="<<testChecks<<" passed\n";}
