#include "parable_libra.h"
#include "activation_guard.h"
#include "test_check.h"
#include "rat_measure.h" // owned existing FFT only; no RAT processor execution
#include <fstream>
#include <sstream>
#include <iostream>
#include <limits>
using parable::dsp::Libra;using parable::dsp::LibraControls;constexpr double pi=3.14159265358979323846;
static std::vector<std::vector<double>> csv(const char* name){std::ifstream f(std::string(LIBRA_FIXTURES)+"/"+name);CHECK(bool(f));std::string s;std::getline(f,s);std::vector<std::vector<double>> out;while(std::getline(f,s)){std::stringstream in(s);std::string field;std::vector<double> r;while(std::getline(in,field,','))r.push_back(std::stod(field));out.push_back(r);}return out;}
static void references(){double voltage=0,current=0,transient=0;auto compare=[&](const auto& p,const auto& r,unsigned k){CHECK(p.valid);voltage=std::max({voltage,std::abs(double(p.driverVolts)-r[k]),std::abs(double(p.combinedVolts)-r[k+1])});current=std::max({current,std::abs(double(p.upperAmps)-r[k+2]),std::abs(double(p.lowerAmps)-r[k+3])});};
    for(const auto& r:csv("bias.csv")){Libra m;LibraControls c;c.damagedBias=r[0]!=0;CHECK(m.prepare(48000,c,4,r[1]!=0));compare(m.bias(),r,2);}
    for(const auto& r:csv("curve.csv")){Libra m;LibraControls c;c.damagedBias=r[0]!=0;CHECK(m.prepare(48000,c,4,r[1]!=0));const auto p=m.dc(float(r[2]));compare(p,r,3);CHECK(std::abs(p.upperAmps-p.lowerAmps-p.combinedVolts/m.effectiveLoad())<1e-6);}
    for(const auto& r:csv("overload.csv")){Libra m;LibraControls c;c.damagedBias=r[0]!=0;CHECK(m.prepare(48000,c));compare(m.dc(float(r[1])),r,2);}
    for(const auto& r:csv("load.csv")){Libra m;LibraControls c;c.damagedBias=r[0]!=0;CHECK(m.prepare(48000,c,4,true,float(r[1])));compare(m.dc(float(r[2])),r,3);}CHECK(voltage<.001&&current<1e-6);
    for(bool fault:{false,true}){constexpr unsigned fs=96000;Libra m;LibraControls c;c.drive=.1f;c.damagedBias=fault;CHECK(m.prepare(fs,c,1));unsigned next=0;
        for(const auto& r:csv("transient.csv"))if(bool(r[0])==fault){const auto target=unsigned(std::llround(r[1]*fs));for(;next<=target;++next)m.process(float(.003*std::sin(2*pi*220*next/fs)));const auto p=m.probe();transient=std::max({transient,std::abs(double(p.driverVolts)-r[2]),std::abs(double(p.combinedVolts)-r[3]),std::abs(double(p.jackVolts)-r[4])});CHECK(!m.failures());}}
    CHECK(transient<.005);std::cout<<"Independent equivalent peak errors: voltage_V="<<voltage<<" branch_A="<<current<<" short_transient_V="<<transient<<'\n';
}
static void feedback(){double errors[2]{};for(unsigned feedback=0;feedback<2;++feedback){Libra h,f;LibraControls c;CHECK(h.prepare(48000,c,4,feedback!=0));c.damagedBias=true;CHECK(f.prepare(48000,c,4,feedback!=0));const double gain=h.dc(.0001f).combinedVolts/.0001,amplitude=.2/gain;double delta=0,energy=0;
        for(unsigned n=0;n<1024;++n){const auto input=float(amplitude*std::sin(2*pi*n/1024));const auto hp=h.dc(input),fp=f.dc(input);const double a=hp.combinedVolts-h.bias().combinedVolts,b=fp.combinedVolts-f.bias().combinedVolts;delta+=(a-b)*(a-b);energy+=a*a;}
        errors[feedback]=std::sqrt(delta/energy);std::cout<<"Matched nominal0.2V DC-sine feedback="<<feedback<<" gain_input_peak_V="<<amplitude<<" fault_difference_relative_RMS="<<errors[feedback]<<" healthy_idle_A="<<h.bias().upperAmps<<" fault_upper/lower_A="<<f.bias().upperAmps<<'/'<<f.bias().lowerAmps<<" fault_DC_V="<<f.bias().combinedVolts<<'\n';}
    CHECK(errors[1]<errors[0]);Libra h,f;LibraControls c;CHECK(h.prepare(48000,c));c.damagedBias=true;CHECK(f.prepare(48000,c));CHECK(h.bias().upperAmps>.0005&&h.bias().lowerAmps>.0005&&f.bias().upperAmps<1e-8&&f.bias().lowerAmps<1e-5);
}
static void alias(){constexpr unsigned fs=48000,size=4096,bin=680;for(bool fault:{false,true}){double spur[3]{};for(unsigned mode=0;mode<3;++mode){Libra m;LibraControls c;c.drive=.9f;c.damagedBias=fault;CHECK(m.prepare(fs,c,1u<<mode));std::vector<std::complex<double>> bins(size);
            for(unsigned n=0;n<fs/2+size;++n){const auto y=m.process(float(.04*std::sin(2*pi*bin*n/size)));if(n>=fs/2){const auto k=n-fs/2;bins[k]=y*(.5-.5*std::cos(2*pi*k/(size-1)));}}ratlab::fft(bins);double largest=0;for(unsigned k=86;k<size/2;++k){bool harmonic=false;for(unsigned h=1;h*bin<size/2;++h)if(std::abs(int(k)-int(h*bin))<=3)harmonic=true;if(!harmonic)largest=std::max(largest,std::abs(bins[k]));}spur[mode]=20*std::log10(largest/std::abs(bins[bin]));CHECK(std::isfinite(spur[mode])&&!m.failures());}
        std::cout<<"7968.75Hz nonharmonic spur fault="<<fault<<" 1x/2x/4x_dBr="<<spur[0]<<'/'<<spur[1]<<'/'<<spur[2]<<" (includes discretization/filter differences)\n";CHECK(spur[2]<spur[0]);}}
static void safety(){activation_guard::selfTest();bool good=true;double residual=0,kcl=0;for(float fs:{44100.f,48000.f,96000.f})for(bool fault:{false,true})for(float load:{100.f,1000.f,10000.f}){Libra m;LibraControls c;c.drive=load==100?.1f:.9f;c.damagedBias=fault;c.level=load==10000?0:.8f;CHECK(m.prepare(fs,c,4,true,load));bool upper=false,lower=false;
        activation_guard::begin();for(unsigned n=0;n<2048;++n){const auto y=m.process(float(.04*std::sin(2*pi*220*n/fs)));const auto p=m.probe();good&=std::isfinite(y)&&std::abs(y)<=1&&p.valid;if(c.level==0)good&=y==0;upper|=p.upperAmps>p.lowerAmps;lower|=p.lowerAmps>p.upperAmps;kcl=std::max(kcl,std::abs(double(p.upperAmps-p.lowerAmps-p.combinedVolts/m.effectiveLoad())));if(!p.railed)residual=std::max(residual,std::abs(double(p.poleResidualVolts)));}activation_guard::end();CHECK(good&&upper&&lower&&activation_guard::clean()&&!m.failures());
        m.reset();std::array<float,128> first{};for(auto& y:first)y=m.process(.01f);m.reset();for(auto y:first)good&=y==m.process(.01f);CHECK(good);auto old=m;CHECK(!m.prepare(NAN,c)&&!m.prepare(fs,{2,.8f,false})&&!m.prepare(fs,c,3)&&!m.prepare(fs,c,4,true,99));CHECK(m.process(.01f)==old.process(.01f));CHECK(!m.dc(NAN).valid&&!m.dc(5).valid);
        const auto before=m.probe();CHECK(m.process(NAN)==0&&m.process(INFINITY)==0&&m.probe().combinedVolts==before.combinedVolts);CHECK(std::isfinite(m.process(std::numeric_limits<float>::max()))&&m.inputClamps()>0);m.stop();CHECK(m.process(.1f)==0);m.reset();for(unsigned n=0;n<4096;++n)good&=std::abs(m.process(0))<1e-7f;CHECK(good&&!m.failures());}
    CHECK(kcl<1e-6&&residual<1e-4);std::cout<<"Finite/load/rate/controls/reset/silence/guard pass; KCL_A="<<kcl<<" implicit_pole_residual_V="<<residual<<" instance_bytes="<<sizeof(Libra)<<'\n';}
int main(){references();feedback();alias();safety();std::cout<<"Libra CHECKs="<<testChecks<<" passed\n";}
