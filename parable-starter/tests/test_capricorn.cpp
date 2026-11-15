#include "parable_capricorn.h"
#include "activation_guard.h"
#include "test_check.h"
#include "rat_measure.h" // existing owned FFT only; no RAT processor execution
#include <fstream>
#include <iostream>
#include <sstream>
#include <limits>
using parable::dsp::Capricorn;using parable::dsp::CapricornControls;
constexpr double pi=3.14159265358979323846;
static std::vector<std::vector<double>> csv(const char* name){std::ifstream f(std::string(CAPRICORN_FIXTURES)+"/"+name);CHECK(bool(f));std::string s;std::getline(f,s);std::vector<std::vector<double>> out;while(std::getline(f,s)){std::stringstream in(s);std::string word;std::vector<double> r;while(std::getline(in,word,','))r.push_back(std::stod(word));out.push_back(r);}return out;}
static void electrical(){Capricorn healthy,broken;CapricornControls fault;fault.openC4=true;CHECK(healthy.prepare(48000,{},4));CHECK(broken.prepare(48000,fault,4));CHECK(healthy.bias()==broken.bias());CHECK(healthy.diodeResistance()==broken.diodeResistance());
    double dc=0,curve=0,clip=0,q2=0,tone=0;for(const auto& r:csv("bias.csv"))for(unsigned k=0;k<3;++k)dc=std::max(dc,std::abs(healthy.bias()[k]-r[k]));CHECK(dc<.005);
    for(const auto& r:csv("curve.csv")){curve=std::max(curve,std::abs(double(healthy.transistorCurve(float(r[0])))-r[1]));CHECK(healthy.transistorCurve(float(r[0]))==broken.transistorCurve(float(r[0])));}CHECK(curve<.02);CHECK(std::abs(healthy.transistorCurve(.1f)+healthy.transistorCurve(-.1f))>.01);
    for(const auto& r:csv("clip.csv")){clip=std::max(clip,std::abs(double(healthy.diodeCurve(float(r[0])))-r[1]));CHECK(std::abs(healthy.diodeCurve(float(r[0]))+healthy.diodeCurve(float(-r[0])))<1e-7);}CHECK(clip<.001);
    for(const auto& r:csv("q2-ac.csv")){constexpr unsigned fs=96000;Capricorn m;CapricornControls c;c.openC4=r[0]!=0;CHECK(m.prepare(fs,c));std::complex<double> sum{};constexpr double amp=1e-6;
        for(unsigned n=0;n<fs*3/10;++n){const double phase=2*pi*r[1]*n/fs;m.process(float(amp*std::sin(phase)));if(n>=fs/5)sum+=double(m.probe().q2Volts)*std::polar(1.,-phase);}
        const auto actual=std::abs(sum)*2/(amp*(fs/10));const double fc=1/(2*pi*472000*10e-9),input=470./472*r[1]/std::sqrt(r[1]*r[1]+fc*fc),expected=std::hypot(r[2],r[3])*input;const double error=std::abs(actual/expected-1);q2=std::max(q2,error);CHECK(error<.03);CHECK(!m.failures()&&!m.tableClamps());std::cout<<"Q2 open="<<r[0]<<" Hz="<<r[1]<<" gain="<<actual<<" reference_with_input="<<expected<<" relative_error="<<error<<'\n';}
    for(const auto& r:csv("tone.csv")){constexpr unsigned fs=48000;Capricorn m;CapricornControls c;c.tone=float(r[0]);CHECK(m.prepare(fs,c));std::complex<double> input{},output{};
        for(unsigned n=0;n<fs*3/10;++n){const double phase=2*pi*r[1]*n/fs;m.process(float(1e-5*std::sin(phase)));if(n>=fs/5){const auto e=std::polar(1.,-phase);input+=double(m.probe().diodeVolts)*e;output+=double(m.probe().jackVolts)*e;}}
        const auto actual=std::abs(output/input),expected=std::hypot(r[2],r[3]);const double error=std::abs(actual/expected-1);tone=std::max(tone,error);CHECK(error<.02);CHECK(!m.failures());}
    Capricorn m;CapricornControls c;c.distortion=.9f;CHECK(m.prepare(48000,c));double diode=0,amp=0;for(unsigned n=0;n<24000;++n){m.process(float(.1*std::sin(2*pi*220*n/48000)));diode=std::max(diode,std::abs(double(m.probe().diodeVolts)));amp=std::max(amp,std::abs(double(m.probe().amplifierVolts)));}CHECK(diode>.5&&diode<.8&&amp<=3&&!m.failures()&&!m.tableClamps());
    std::cout<<"Peak errors: DC_V="<<dc<<" Q2_curve_V="<<curve<<" diode_V="<<clip<<" Q2_AC_relative="<<q2<<" Tone_relative="<<tone<<" actual_clip_peak_V="<<diode<<" amp_peak_V="<<amp<<'\n';
}
static void alias(){constexpr unsigned fs=48000,size=4096,bin=680;for(bool fault:{false,true}){double spur[3]{};for(unsigned mode=0;mode<3;++mode){Capricorn m;CapricornControls c;c.distortion=.9f;c.openC4=fault;CHECK(m.prepare(fs,c,mode==2?8:mode?4:1));std::vector<std::complex<double>> bins(size);
            for(unsigned n=0;n<fs/2+size;++n){const auto y=m.process(float(.04*std::sin(2*pi*bin*n/size)));if(n>=fs/2){const auto k=n-fs/2;bins[k]=y*(.5-.5*std::cos(2*pi*k/(size-1)));}}ratlab::fft(bins);
            double largest=0;for(unsigned k=86;k<size/2;++k){bool harmonic=false;for(unsigned h=1;h*bin<size/2;++h)if(std::abs(int(k)-int(h*bin))<=3)harmonic=true;if(!harmonic)largest=std::max(largest,std::abs(bins[k]));}spur[mode]=20*std::log10(largest/std::abs(bins[bin]));CHECK(std::isfinite(spur[mode])&&!m.failures()&&!m.tableClamps());}
        std::cout<<"7968.75Hz nonharmonic spur open="<<fault<<" 1x/4x/8x_dBr="<<spur[0]<<'/'<<spur[1]<<'/'<<spur[2]<<" (observation, not a measured-pedal match)\n";CHECK(spur[1]<spur[0]&&spur[2]<spur[1]);}}
static void safety(){activation_guard::selfTest();bool good=true;for(float fs:{44100.f,48000.f,96000.f})for(bool fault:{false,true})for(unsigned corner=0;corner<5;++corner){CapricornControls c;c.openC4=fault;c.distortion=corner==0?0:corner==1?1:.5f;c.tone=corner==2?0:corner==3?1:.5f;c.level=corner==4?0:.8f;Capricorn m;CHECK(m.prepare(fs,c));
        activation_guard::begin();for(unsigned n=0;n<1024;++n){const auto y=m.process(float(.1*std::sin(2*pi*3000*n/fs)));good&=std::isfinite(y)&&std::abs(y)<=1;if(c.level==0)good&=y==0;}activation_guard::end();CHECK(good&&activation_guard::clean()&&!m.failures()&&!m.tableClamps());
        m.reset();std::array<float,128> first{};for(auto& y:first)y=m.process(.02f);m.reset();for(auto y:first)good&=m.process(.02f)==y;CHECK(good);auto copy=m;CHECK(!m.prepare(NAN,c)&&!m.prepare(fs,{2,.5f,.5f})&&!m.prepare(fs,c,2));CHECK(m.process(.01f)==copy.process(.01f));
        const auto before=m.probe();CHECK(m.process(NAN)==0&&m.process(INFINITY)==0&&m.probe().diodeVolts==before.diodeVolts);CHECK(std::isfinite(m.process(std::numeric_limits<float>::max()))&&m.inputClamps()>0);m.stop();CHECK(m.process(.1f)==0);m.reset();for(unsigned n=0;n<2048;++n)good&=m.process(0)==0;CHECK(good);}
    std::cout<<"Rate/control/endpoint/finite/reset/silence/rejected-prepare/callback conditions pass; instance_bytes="<<sizeof(Capricorn)<<'\n';}
int main(){electrical();alias();safety();std::cout<<"Capricorn CHECKs="<<testChecks<<" passed\n";}
