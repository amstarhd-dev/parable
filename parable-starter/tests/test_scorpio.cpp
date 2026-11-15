#include "parable_scorpio.h"
#include "activation_guard.h"
#include "test_check.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>
using parable::dsp::Scorpio;using parable::dsp::ScorpioControls;
constexpr double pi=3.14159265358979323846;
static std::vector<std::vector<double>> csv(const char* name){std::ifstream f(std::string(SCORPIO_FIXTURES)+"/"+name);CHECK(bool(f));std::string s;std::getline(f,s);std::vector<std::vector<double>> rows;
    while(std::getline(f,s)){std::stringstream stream(s);std::string field;std::vector<double> row;while(std::getline(stream,field,','))row.push_back(std::stod(field));rows.push_back(row);}return rows;}
static void electrical(){double biasError=0,curveError=0,acError=0;
    for(const auto& row:csv("bias.csv")){Scorpio m;ScorpioControls c;c.resistorDrift=row[0]!=0;CHECK(m.prepare(48000,c));for(unsigned k=0;k<5;++k)biasError=std::max(biasError,std::abs(m.bias()[k]-row[k+1]));}CHECK(biasError<.005);
    for(bool fault:{false,true})for(float gain:{.1f,.9f}){Scorpio m;ScorpioControls c;c.gain=gain;c.resistorDrift=fault;CHECK(m.prepare(48000,c));
        for(const auto& r:csv("curve.csv"))if(bool(r[0])==fault&&std::abs(r[1]-gain)<1e-6)curveError=std::max(curveError,std::abs(m.curve(float(r[2]))-r[3]));}CHECK(curveError<.02);
    for(const auto& r:csv("ac.csv")){constexpr unsigned fs=192000;constexpr double amp=1e-6;Scorpio m;ScorpioControls c;c.gain=float(r[1]);c.resistorDrift=r[0]!=0;CHECK(m.prepare(fs,c,1,true,false));
        std::complex<double> sum{};for(unsigned n=0;n<fs*3/10;++n){const double phase=2*pi*r[2]*n/fs;m.process(float(amp*std::sin(phase)));if(n>=fs/5)sum+=double(m.probe().preampVolts)*std::polar(1.,-phase);}
        const auto actual=sum*std::complex<double>(0,2/(amp*(fs/10)));const std::complex<double> expected(r[3],r[4]);const double error=std::abs(std::abs(actual)/std::abs(expected)-1);acError=std::max(acError,error);
        std::cout<<"AC "<<r[0]<<' '<<r[2]<<" actual="<<actual<<" expected="<<expected<<" magnitude_error="<<error<<'\n';CHECK(error<.05);CHECK(m.failures()==0);}
    Scorpio m;ScorpioControls c;c.gain=.1f;CHECK(m.prepare(192000,c,1,true,false));unsigned next=0;double transientPeak=0;
    for(const auto& r:csv("transient.csv")){const unsigned target=unsigned(std::llround(r[0]*192000));for(;next<=target;++next)m.process(float(1e-4*std::sin(2*pi*1000*next/192000)));
        const double e=std::abs(m.probe().outputVolts-r[1]);transientPeak=std::max(transientPeak,e);CHECK(e<.001+.05*std::abs(r[1]));}
    std::cout<<"DC peakV="<<biasError<<" curve peakV="<<curveError<<" AC relative="<<acError<<" transient peakV="<<transientPeak<<'\n';
}
// Owned radix2 FFT already used in the main Soundforge laboratories.
static void fft(std::vector<std::complex<double>>& a){const auto n=a.size();for(std::size_t i=1,j=0;i<n;++i){auto bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t length=2;length<=n;length<<=1){const auto root=std::polar(1.,-2*pi/length);for(std::size_t i=0;i<n;i+=length){std::complex<double> w=1;for(std::size_t j=0;j<length/2;++j){const auto u=a[i+j],v=a[i+j+length/2]*w;a[i+j]=u+v;a[i+j+length/2]=u-v;w*=root;}}}}
static void alias(){constexpr unsigned fs=48000,size=4096,bin=680;
    for(bool fault:{false,true}){double spur[2]{};for(unsigned mode=0;mode<2;++mode){Scorpio m;ScorpioControls c;c.gain=.9f;c.resistorDrift=fault;CHECK(m.prepare(fs,c,mode?4:1,true,mode!=0));std::vector<std::complex<double>> bins(size);
            for(unsigned n=0;n<fs*2+size;++n){const auto y=m.process(float(.02*std::sin(2*pi*bin*n/size)));if(n>=fs*2){const auto j=n-fs*2;bins[j]=y*(.5-.5*std::cos(2*pi*j/(size-1)));}}
            fft(bins);double maximum=0;for(unsigned k=86;k<size/2;++k){bool wanted=false;for(unsigned h=1;h*bin<size/2;++h)if(std::abs(int(k)-int(h*bin))<=3)wanted=true;if(!wanted)maximum=std::max(maximum,std::abs(bins[k]));}
            spur[mode]=20*std::log10(std::max(maximum,1e-20)/std::abs(bins[bin]));CHECK(!m.failures()&&!m.tableClamps());}
        std::cout<<"High-note spur fault="<<fault<<" 1x/4x_dBr="<<spur[0]<<'/'<<spur[1]<<'\n';CHECK(spur[1]<spur[0]-6);}
}
static void magnetics(){Scorpio nonlinear,linear;ScorpioControls c;c.gain=.9f;c.lowCut=0;CHECK(nonlinear.prepare(48000,c));CHECK(linear.prepare(48000,c,4,false));double difference=0,nonlinearCurrent=0,energy=0;
    for(unsigned n=0;n<48000;++n){const float x=float(.12*std::sin(2*pi*40*n/48000));const auto a=nonlinear.process(x),b=linear.process(x);difference+=double(a-b)*(a-b);const auto p=nonlinear.probe();nonlinearCurrent=std::max(nonlinearCurrent,std::abs(double(p.magnetizingAmps)-p.flux/20));const double phi=p.flux;energy=std::max(energy,phi*phi/40+phi*phi*phi*phi/(80*.008*.008));}
    CHECK(difference>1e-5&&nonlinearCurrent>1e-7&&energy>0);CHECK(!nonlinear.failures()&&!linear.failures());std::cout<<"Magnetic linear/nonlinear RMS_difference="<<std::sqrt(difference/48000)<<" nonlinear_current_A="<<nonlinearCurrent<<" max_stored_J="<<energy<<'\n';
}
static void safety(){activation_guard::selfTest();Scorpio m;CHECK(m.process(.1f)==0);bool good=true;
    for(float fs:{44100.f,48000.f,96000.f})for(bool fault:{false,true})for(unsigned setting=0;setting<3;++setting){ScorpioControls c;c.gain=setting*.5f;c.lowCut=setting*.5f;c.level=setting==1?0:.8f;c.resistorDrift=fault;CHECK(m.prepare(fs,c));
        activation_guard::begin();for(unsigned n=0;n<2048;++n){const auto y=m.process(float(.03*std::sin(2*pi*220*n/fs)));good&=std::isfinite(y)&&std::abs(y)<=1;if(c.level==0)good&=y==0;}activation_guard::end();CHECK(good&&activation_guard::clean()&&!m.failures());
        m.reset();std::array<float,128> first{};for(auto& y:first)y=m.process(.01f);m.reset();for(auto y:first)good&=y==m.process(.01f);CHECK(good);
        auto copy=m;CHECK(!m.prepare(NAN,c));CHECK(!m.prepare(fs,{2,.5f,.5f}));CHECK(!m.prepare(fs,c,2));for(unsigned n=0;n<16;++n)good&=m.process(.01f)==copy.process(.01f);CHECK(good);
        const auto before=m.probe();CHECK(m.process(NAN)==0&&m.probe().flux==before.flux);CHECK(m.process(INFINITY)==0);CHECK(std::isfinite(m.process(std::numeric_limits<float>::max())));CHECK(m.inputClamps()>0);
        m.stop();CHECK(m.process(1)==0);m.reset();for(unsigned n=0;n<4096;++n)good&=m.process(0)==0;CHECK(good&&!m.failures());}
    std::cout<<"Rate/control corners, finite stress, rejected prepare, reset/silence and callback guards pass; state="<<sizeof(Scorpio)<<" bytes\n";
}
int main(){electrical();magnetics();alias();safety();std::cout<<"Scorpio checks="<<testChecks<<" passed\n";}
