#include "parable_wavefolder.h"
#include "wavefolder_cell.h"
#include "activation_guard.h"
#include "test_check.h"
#include <complex>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
using parable::dsp::Wavefolder;
namespace cell=parable::dsp::wavefolder_detail;
constexpr double pi=3.14159265358979323846;
using Rows=std::vector<std::vector<std::string>>;
static Rows csv(const char* name){std::ifstream f(std::string(WAVEFOLDER_FIXTURES)+"/"+name);CHECK(bool(f));Rows r;std::string line,s;std::getline(f,line);while(std::getline(f,line)){std::stringstream in(line);std::vector<std::string> row;while(std::getline(in,s,','))row.push_back(s);r.push_back(row);}return r;}
static double v(const std::vector<std::string>& r,unsigned n){return std::stod(r[n]);}
static void fft(std::vector<std::complex<double>>& a){const auto n=a.size();for(std::size_t i=1,j=0;i<n;++i){auto bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}for(std::size_t len=2;len<=n;len<<=1){const auto root=std::polar(1.,-2*pi/len);for(std::size_t i=0;i<n;i+=len){std::complex<double>w=1;for(std::size_t j=0;j<len/2;++j){const auto u=a[i+j],b=a[i+j+len/2]*w;a[i+j]=u+b;a[i+j+len/2]=u-b;w*=root;}}}}
static void science(){
 double nodes=0,currents=0,table=0,kcl=0,fold=0;Wavefolder h,f;CHECK(h.prepare(48000));CHECK(f.prepare(48000,{1,true}));
 for(const char* name:{"dc.csv","load-rail.csv"})for(const auto& r:csv(name)){
  const auto p=cell::solve(v(r,3),v(r,0),v(r,1),v(r,2));CHECK(p.valid);for(unsigned i=0;i<3;++i){const unsigned col=i==0?5:i==1?6:4;const double e=std::abs(p.nodes[i]-v(r,col));nodes=std::max(nodes,e);CHECK(e<2e-6);}
  const auto& j=p.junction;const double icp=-j[0]+j[1]/cell::ar,icn=j[2]-j[3]/cell::ar;
  currents=std::max({currents,std::abs(icp-v(r,7)),std::abs(icn-v(r,8))});CHECK(currents<2e-9);CHECK(cell::norm(cell::evaluate(p.nodes,v(r,3),v(r,0),v(r,1),v(r,2)).residual)<1e-9);
  if(std::string(name)=="dc.csv"&&v(r,0)!=33000){const auto q=(v(r,0)==15000?h:f).cellCurve(v(r,3));table=std::max(table,std::abs(q.volts+v(r,4)));CHECK(table<2e-5);cell::Vec observed{};const auto s=cell::observe(v(r,3),q.volts,observed,v(r,0));CHECK(s.valid);kcl=std::max(kcl,cell::norm(s.residual));CHECK(kcl<2e-7);}
 }
 for(const auto& r:csv("candidates.csv")){const double x=v(r,1);auto a=cell::solve(x-.001,v(r,0)),b=cell::solve(x+.001,v(r,0));CHECK(a.valid&&b.valid&&a.slope*b.slope<0);double lo=x-.001,hi=x+.001;for(unsigned n=0;n<30;++n){const double mid=(lo+hi)/2;const auto p=cell::solve(mid,v(r,0));CHECK(p.valid);if(p.slope*a.slope>0)lo=mid;else hi=mid;}fold=std::max(fold,std::abs((lo+hi)/2-x));CHECK(fold<.001);}
 CHECK(std::abs(h.zeroInputVolts())<1e-10);CHECK(std::abs(f.zeroInputVolts()-.793664972)<2e-6);
 // Derivative of exactly the rounded cubic's primitive, not a second curve.
 for(bool fault:{false,true}){const auto& m=fault?f:h;for(unsigned i=0;i<6000;++i){const double x=-2.9995+i*.001;const auto q=m.cellCurve(x);const double derivative=(m.cellCurve(x+1e-6).primitive-m.cellCurve(x-1e-6).primitive)/2e-6;CHECK(std::abs(derivative-q.volts)<2e-8);if(!fault)CHECK(std::abs(q.volts+m.cellCurve(-x).volts)<1e-6);}}
 for(const auto& r:csv("harmonics.csv")){const auto& m=v(r,0)==15000?h:f;constexpr unsigned n=65536;std::complex<double>s[6]{};double sum=0,squares=0;const unsigned hs[]={1,2,3,5,7,9};for(unsigned i=0;i<n;++i){const double angle=2*pi*i/n,y=m.cellCurve(v(r,2)+v(r,1)*std::sin(angle)).volts;sum+=y;squares+=y*y;for(unsigned k=0;k<6;++k)s[k]+=y*std::polar(1.,-hs[k]*angle);}CHECK(std::abs(sum/n-v(r,3))<2e-5);CHECK(std::abs(std::sqrt(squares/n)-v(r,4))<2e-5);for(unsigned k=0;k<6;++k)CHECK(std::abs(2*std::abs(s[k])/n-v(r,5+k))<2e-5);}
 std::cout<<"SPICE nodes_peak_V="<<nodes<<" currents_peak_A="<<currents<<" LUT_peak_V="<<table<<" actual_LUT_KCL_A="<<kcl<<" folds_peak_V="<<fold<<'\n';
}
static std::vector<float> excitation(unsigned rate,unsigned kind){constexpr unsigned n=4096;const unsigned bin=(kind==0?1760:kind==1?3520:kind==2?8000:1000)*n/rate;std::vector<float>x(n);double peak=0;for(unsigned i=0;i<n;++i){double a=0;if(kind<3)a=std::sin(2*pi*bin*i/n);else for(unsigned b:{76u,97u,113u})for(unsigned h=1;h<=5;++h)a+=std::sin(2*pi*b*h*i/n)/(h*h);peak=std::max(peak,std::abs(a));x[i]=float(a);}for(auto& a:x)a=float(a*.2/peak);return x;}
static void alias(){
 const auto rows=csv("reference.csv");std::cout<<"rate,fault,kind,ADAA,relative_RMS,peak_error_spectral_component_normalized,peak_error_time_normalized,unwanted_spur_dBr_reference_RMS,wall_us_per_frame\n";
 constexpr unsigned n=4096;CHECK(rows.size()==24*n);
 for(unsigned group=0;group<24;++group){const auto& first=rows[group*n];const unsigned rate=unsigned(v(first,0)),fault=unsigned(v(first,1)),kind=unsigned(v(first,2));auto x=excitation(rate,kind);std::vector<std::complex<double>> expected(n);for(unsigned i=0;i<n;++i){CHECK(unsigned(v(rows[group*n+i],3))==i);expected[i]=v(rows[group*n+i],4);}fft(expected);double energy=0;for(const auto& a:expected)energy+=std::norm(a);CHECK(energy>1e-8);
  for(bool aa:{false,true}){Wavefolder m;CHECK(m.prepare(float(rate),{6,bool(fault)},4,aa));std::vector<std::complex<double>> actual(n);for(unsigned i=0;i<8*n;++i){const float y=m.process(x[i%n]);if(i>=7*n)actual[i%n]=y;}fft(actual);double error=0,peak=0,spur=0;std::vector<std::complex<double>> difference(n);
   for(unsigned i=0;i<n;++i){const double omega=2*pi*(i<=n/2?double(i):double(i)-n)/n;const auto a=actual[i]*std::polar(1.,aa?omega*.5/4:0);difference[i]=a-expected[i];error+=std::norm(difference[i]);peak=std::max(peak,std::abs(a-expected[i])/n);}
   if(kind<3){const unsigned bin=(kind==0?1760:kind==1?3520:8000)*n/rate;for(unsigned i=1;i<n/2;++i)if(i%bin)spur=std::max(spur,2*std::abs(actual[i])/n);}
   fft(difference);double timePeak=0;for(const auto& d:difference)timePeak=std::max(timePeak,std::abs(d.real())/n);
   const double rms=std::sqrt(energy)/n,relative=std::sqrt(error/energy),db=20*std::log10(std::max(spur,1e-30)/rms);
   std::cout<<rate<<','<<fault<<','<<kind<<','<<aa<<','<<relative<<','<<peak<<','<<timePeak<<','<<db<<",not-timed-here\n";
   if(aa){CHECK(relative<(kind==2?.05:.01));if(kind<3)CHECK(db< -60);}CHECK(!m.failures()&&!m.inputClamps()&&!m.tableClamps()&&!m.outputClamps());
  }
 }
}
static void gainSweeps(){
 // Controlled actual processor, not static-law harmonics relabeled as audio.
 std::cout<<"harmonics:rate,fault,actual_Hz,drive,H1,H2,H3,H5\n";
 for(unsigned rate:{44100u,48000u,96000u})for(bool fault:{false,true})for(unsigned kind:{0u,1u})for(float drive:{1.f,3.f,6.f}){auto x=excitation(rate,kind);Wavefolder m;CHECK(m.prepare(float(rate),{drive,fault}));constexpr unsigned n=4096;std::vector<std::complex<double>> y(n);for(unsigned i=0;i<8*n;++i){const auto a=m.process(x[i%n]);if(i>=7*n)y[i%n]=a;}fft(y);const unsigned bin=(kind?3520:1760)*n/rate;std::cout<<"harmonics:"<<rate<<','<<fault<<','<<double(bin)*rate/n<<','<<drive;for(unsigned h:{1u,2u,3u,5u})std::cout<<','<<(h*bin<n/2?2*std::abs(y[h*bin])/n:0);std::cout<<'\n';CHECK(!m.failures()&&!m.tableClamps()&&!m.outputClamps());}
}
static void safety(){namespace g=activation_guard;g::selfTest();
 Wavefolder empty;CHECK(empty.process(.2f)==0);CHECK(!empty.prepare(44099));CHECK(!empty.prepare(96001));CHECK(!empty.prepare(48000,{7,false}));CHECK(!empty.prepare(48000,{},3));
 for(float rate:{44100.f,48000.f,96000.f})for(bool fault:{false,true})for(unsigned factor:{1u,2u,4u})for(bool aa:{false,true}){Wavefolder m;CHECK(m.prepare(rate,{6,fault},factor,aa));g::begin();for(unsigned i=0;i<20000;++i){const float y=m.process(i<1000?0.f:(i&1?.25f:-.25f));CHECK(std::isfinite(y)&&std::abs(y)<=1);if(i<1000)CHECK(std::abs(y)<1e-8);}g::end();CHECK(g::clean()&&!m.failures()&&!m.tableClamps()&&!m.outputClamps());
  auto saved=m;CHECK(!m.prepare(std::numeric_limits<float>::quiet_NaN()));CHECK(!m.prepare(48000,{std::numeric_limits<float>::infinity(),false}));CHECK(!m.setDrive(-1));CHECK(!m.setDrive(std::numeric_limits<float>::quiet_NaN()));
  CHECK(m.process(std::numeric_limits<float>::infinity())==0);CHECK(m.process(std::numeric_limits<float>::quiet_NaN())==0);CHECK(m.invalidInputs()==2);for(unsigned i=0;i<500;++i)CHECK(m.process(.1f)==saved.process(.1f));
  m.reset();auto repeat=m;for(unsigned i=0;i<200;++i)CHECK(m.process(.12f)==repeat.process(.12f));m.reset();CHECK(m.setDrive(0));for(unsigned i=0;i<1000;++i)CHECK(std::abs(m.process(.2f))<1e-8);CHECK(m.setDrive(6));
  g::begin();for(unsigned i=0;i<4000;++i){const float y=m.process(i<2000?std::numeric_limits<float>::max():-std::numeric_limits<float>::max());CHECK(std::isfinite(y)&&std::abs(y)<=1);}g::end();CHECK(g::clean()&&m.inputClamps()==4000&&!m.tableClamps()&&!m.failures());
  m.reset();for(unsigned i=0;i<500;++i)m.process(.2f);for(unsigned i=0;i<100000;++i)m.process(0);CHECK(std::abs(m.probe().outputVolts)<1e-8);m.stop();CHECK(m.process(.2f)==0);m.reset();CHECK(std::abs(m.process(0))<1e-8);
 }
 std::cout<<"Guard/lifecycle/rates/factors/AA/dynamic-drive/extremes passed; instance_bytes="<<sizeof(Wavefolder)<<'\n';
}
int main(){science();alias();gainSweeps();safety();}
