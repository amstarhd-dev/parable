#include "parable_fz1.h"
#include "germanium_device.h"
#include "activation_guard.h"
#include "test_check.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
using parable::dsp::Fz1;using parable::dsp::Fz1Config;
constexpr double pi=3.14159265358979323846;
using Rows=std::vector<std::vector<double>>;
static Rows csv(const std::string& name){std::ifstream f(std::string(FZ1_FIXTURES)+"/"+name);CHECK(bool(f));Rows rows;std::string line,s;std::getline(f,line);while(std::getline(f,line)){std::stringstream in(line);std::vector<double> r;while(std::getline(in,s,','))r.push_back(std::stod(s));rows.push_back(r);}return rows;}
static void dc(){
 double peak=0,currentPeak=0,kcl=0,builtin=0,builtinCurrent=0;
 for(const char* name:{"equations-dc.csv","dc.csv"})for(const auto& r:csv(name)){
  Fz1Config c;c.openBiasFeed=bool(r[0]);c.attack=float(r[1]);c.supplyVolts=r[2];c.leakageScale=r[3];Fz1 m;CHECK(m.prepare(48000,c));const auto n=m.biasNodes();
  const bool exact=std::string(name)=="equations-dc.csv";
  for(unsigned i=0;i<6;++i){const double e=std::abs(n[i]-r[4+i]);if(exact){peak=std::max(peak,e);CHECK(e<=2e-6);}else builtin=std::max(builtin,e);}
  for(unsigned q=0;q<3;++q){const double e=std::max(std::abs(m.collectorAmps(q)-r[10+q]),std::abs(m.baseAmps(q)-r[13+q]));if(exact){currentPeak=std::max(currentPeak,e);CHECK(e<=2e-9);}else builtinCurrent=std::max(builtinCurrent,e);}
  const double rg=2200+22000*50000*c.attack/(22000+50000*c.attack);
  const double residuals[]={n[0]/1e6+m.baseAmps(0),n[1]/1e4-m.collectorAmps(0)-m.baseAmps(0),n[2]/rg+(c.openBiasFeed?0:(n[2]+c.supplyVolts)/470e3)+m.baseAmps(1),(n[3]+c.supplyVolts)/1500+m.collectorAmps(1),n[4]/1e4+m.baseAmps(2),(n[5]+c.supplyVolts)/1e4+m.collectorAmps(2)};
  for(double e:residuals){kcl=std::max(kcl,std::abs(e));CHECK(std::abs(e)<2e-9);}
 }
 // Builtin's reverse cubic continuation is independently reported, not blessed
 // by replacing the2uV scientific gate. Exact SPICE MNA is the equation check.
 std::cout<<"DC exact_MNA_peak_V="<<peak<<" current_peak_A="<<currentPeak<<" KCL_A="<<kcl<<" builtin_peak_V="<<builtin<<" builtin_current_A="<<builtinCurrent<<" builtin_2uV_gate="<<(builtin<=2e-6?"PASS":"MISS")<<'\n';
}
static void derivatives(){namespace d=parable::dsp::germanium_detail;double peak=0;for(double leakage:{.5,1.,1.5})for(double be:{-.1,.02,.1,.2})for(double bc:{-3.,-1.,0.,.1}){
 parable::dsp::GermaniumDevice p;p.bcLeakageAmps*=leakage;const auto a=d::device(be,bc,p);CHECK(a.valid);
 for(unsigned j=0;j<2;++j){const double h=1e-7;const auto lo=d::device(be-(j==0?h:0),bc-(j==1?h:0),p),hi=d::device(be+(j==0?h:0),bc+(j==1?h:0),p);const double observed[]={(hi.collector-lo.collector)/(2*h),(hi.base-lo.base)/(2*h)},expected[]={j==0?a.collectorE:a.collectorC,j==0?a.baseE:a.baseC};for(unsigned k=0;k<2;++k){const double e=std::abs(observed[k]-expected[k]);peak=std::max(peak,e);CHECK(e<=1e-9+1e-4*std::abs(expected[k]));}}
 }std::cout<<"derivative_peak_absolute_A_per_V="<<peak<<'\n';}
static double input(unsigned sample,const std::string& kind,double level){const double t=sample/192000.;if(kind=="pluck"){const double age=t-.01;return age<0?0:level*(1-std::exp(-age*3000))*std::exp(-age/.12)*std::sin(2*pi*110*age);}return level*std::sin(2*pi*196*t);}
static void transient(){
 // Literal, independently generated suite. No JSON/runtime dependency in DSP.
 double jackWorst=0,nodeWorst=0;unsigned count=0;
 for(unsigned fault:{0u,1u})for(unsigned caseNumber=0;caseNumber<9;++caseNumber){
  Fz1Config c;c.openBiasFeed=bool(fault);std::string name,kind="sine";double level=.1;
  if(caseNumber<3){c.attack=caseNumber==0?.1f:caseNumber==1?.5f:1.f;kind="pluck";level=.2;name="pluck-"+std::to_string(fault)+"-"+(caseNumber==0?"0.1":caseNumber==1?"0.5":"1");}
  else if(caseNumber<6){level=caseNumber==3?.02:caseNumber==4?.1:.2;name="sine-"+std::to_string(fault)+"-"+(caseNumber==3?"0.02":caseNumber==4?"0.1":"0.2");}
  else if(caseNumber==6){c.sourceOhms=10000;c.loadOhms=100e3;name="loaded-"+std::to_string(fault);}
  else {c.volume=caseNumber==7?0.f:1.f;name="volume-"+std::to_string(fault)+"-"+(caseNumber==7?"0":"1");}
  for(bool exact:{false,true}){
   const auto rows=csv((exact?"equations-":"")+name+".csv");CHECK(!rows.empty());Fz1 m;CHECK(m.prepare(192000,c,1));std::array<double,7> err{},energy{},denseErr{},denseEnergy{};unsigned uniformCount=0,denseCount=0;const auto bias=m.biasNodes();std::size_t at=0;
   for(unsigned sample=0;sample<=unsigned(rows.back()[0]);++sample){m.process(float(input(sample,kind,level)));if(sample!=unsigned(rows[at][0]))continue;const auto n=m.nodes();const auto& r=rows[at];CHECK(std::abs(input(sample,kind,level)-r[2])<1e-6);for(unsigned j=0;j<7;++j){const double a=j==6?m.jackVolts():n[j],b=r[3+j],e=a-b;const double ac=j==6?b:b-bias[j];if(sample%48==0){err[j]+=e*e;energy[j]+=ac*ac;}if(sample>=6000&&sample<=8048){denseErr[j]+=e*e;denseEnergy[j]+=ac*ac;}}if(sample%48==0)++uniformCount;if(sample>=6000&&sample<=8048)++denseCount;++at;if(at==rows.size())break;}
   CHECK(at==rows.size()&&uniformCount>0&&denseCount==2049);CHECK(!m.failures()&&!m.inputClamps()&&!m.outputClamps());CHECK(m.maximumEvaluations()<=8*Fz1::kUpdates+1);
   for(unsigned j=0;j<7;++j){const double e=std::sqrt(err[j]/uniformCount),rms=std::sqrt(energy[j]/uniformCount);if(j==6)jackWorst=std::max(jackWorst,e/(rms+1e-30));else nodeWorst=std::max(nodeWorst,e);CHECK(e<=.02*rms+(j==6?20e-6:.5e-3));const double de=std::sqrt(denseErr[j]/denseCount),dr=std::sqrt(denseEnergy[j]/denseCount);CHECK(de<=.02*dr+(j==6?20e-6:.5e-3));}
   std::cout<<"transient "<<(exact?"equations-":"builtin-")<<name<<" jack_error_V="<<std::sqrt(err[6]/uniformCount)<<" relative="<<std::sqrt(err[6]/std::max(energy[6],1e-30))<<" dense_jack_RMS_error_V="<<std::sqrt(denseErr[6]/denseCount)<<" max_evaluations="<<m.maximumEvaluations()<<'\n';++count;
  }
 }
 std::cout<<"transient_cases="<<count<<" jack_worst_relative="<<jackWorst<<" node_worst_RMS_V="<<nodeWorst<<'\n';
}
static void safety(){namespace g=activation_guard;g::selfTest();Fz1 empty;CHECK(empty.process(.1f)==0);CHECK(!empty.prepare(44099));CHECK(!empty.prepare(96001,{},4));CHECK(!empty.prepare(48000,{},3));
 for(float rate:{44100.f,48000.f,96000.f})for(bool fault:{false,true})for(unsigned factor:{1u,2u,4u,8u}){
  Fz1 m;Fz1Config c;c.openBiasFeed=fault;CHECK(m.prepare(rate,c,factor));g::begin();for(unsigned i=0;i<10000;++i){const float y=m.process(i<1000?0.f:float(.2*std::sin(2*pi*1318*i/rate)));CHECK(std::isfinite(y)&&std::abs(y)<1);if(i<1000)CHECK(std::abs(y)<=1e-8);}g::end();CHECK(g::clean()&&!m.failures()&&!m.inputClamps()&&!m.outputClamps());
  auto saved=m;Fz1Config bad=c;bad.attack=std::numeric_limits<float>::quiet_NaN();CHECK(!m.prepare(rate,bad));bad=c;bad.volume=1.1f;CHECK(!m.prepare(rate,bad));bad=c;bad.supplyVolts=std::numeric_limits<double>::infinity();CHECK(!m.prepare(rate,bad));bad=c;bad.sourceOhms=0;CHECK(!m.prepare(rate,bad));bad=c;bad.loadOhms=0;CHECK(!m.prepare(rate,bad));bad=c;bad.leakageScale=0;CHECK(!m.prepare(rate,bad));CHECK(!m.prepare(std::numeric_limits<float>::quiet_NaN()));for(unsigned i=0;i<100;++i)CHECK(m.process(.03f)==saved.process(.03f));
  g::begin();for(float x:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::denorm_min(),1e20f,-1e20f})CHECK(std::isfinite(m.process(x)));g::end();CHECK(g::clean()&&m.invalidInputs()==3&&m.inputClamps()==2);m.reset();saved.reset();for(unsigned i=0;i<3000;++i){const float x=float(.1*std::sin(i*.1));CHECK(m.process(x)==saved.process(x));}m.stop();CHECK(m.process(.1f)==0);m.reset();CHECK(m.failures()==0);
 }
 for(float attack:{0.f,.5f,1.f})for(float volume:{0.f,1.f})for(bool fault:{false,true})for(double supply:{2.7,3.3})for(double leakage:{.5,1.5}){Fz1 m;Fz1Config c;c.attack=attack;c.volume=volume;c.openBiasFeed=fault;c.supplyVolts=supply;c.leakageScale=leakage;c.sourceOhms=10000;c.loadOhms=1e5;CHECK(m.prepare(48000,c));g::begin();for(unsigned i=0;i<4000;++i){const float y=m.process(float(.45*std::sin(2*pi*196*i/48000)));CHECK(std::isfinite(y));if(volume==0)CHECK(y==0);}g::end();CHECK(g::clean()&&!m.failures()&&!m.inputClamps()&&!m.outputClamps());}
 for(float rate:{44100.f,48000.f,96000.f})for(bool fault:{false,true}){Fz1 m;Fz1Config c;c.openBiasFeed=fault;CHECK(m.prepare(rate,c));g::begin();for(unsigned i=0;i<6000;++i)CHECK(std::isfinite(m.process(i<500?0.f:(i%30<15?.5f:-.5f))));g::end();CHECK(g::clean()&&!m.failures()&&!m.inputClamps()&&!m.outputClamps());}
 // Long release retains real capacitor histories and reaches silence naturally.
 for(bool fault:{false,true}){Fz1 m;Fz1Config c;c.openBiasFeed=fault;CHECK(m.prepare(48000,c));double tail=0;g::begin();for(unsigned i=0;i<4*48000;++i){const float x=i<4800?float(.2*std::sin(2*pi*110*i/48000)):0.f;const double y=m.process(x);if(i>3*48000)tail=std::max(tail,std::abs(y));}g::end();CHECK(g::clean()&&!m.failures());CHECK(tail<1e-5);std::cout<<"long_tail_normalized="<<tail<<'\n';}
}
int main(int argc,char** argv){std::cout<<std::setprecision(12);dc();derivatives();if(argc==1||std::string(argv[1])!="--dc-only"){transient();safety();}std::cout<<"FZ1 checks="<<testChecks<<" state_bytes="<<sizeof(Fz1)<<'\n';}
