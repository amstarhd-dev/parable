#include "perf_bridge.h"
#include "germanium_device_fast.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace parable::dsp;
using namespace parable::dsp::germanium_detail;
constexpr double pi=3.14159265358979323846;
struct Errors {
 double value=0,derivative=0,valueAbsolute=0,derivativeAbsolute=0,derivativeRelative=0,boundaryValue=0,boundaryDerivative=0;
 double current=0,jacobian=0,spiceCurrent=0,output=0,outputRms=0,nodes=0,bias=0,input=0,fundamental=0,harmonic=0,thdn=0,impedance=0,work=0;
 unsigned events=0,configuration=0,invalid=0;std::uint64_t points=0,frames=0;
 std::vector<std::string> cases;
};
void maximum(double& target,double value){target=std::isfinite(value)?std::max(target,value):INFINITY;}
template<class E> void exponential(Errors& e){
 auto check=[&](double z){const auto a=E::evaluate(z);double v=std::expm1(z),d=std::exp(z);maximum(e.value,std::abs(a.value-v)/(5e-12*std::max(1.,std::abs(v))));maximum(e.derivative,std::abs(a.derivative-d)/(5e-16+5e-9*d));maximum(e.valueAbsolute,std::abs(a.value-v));maximum(e.derivativeAbsolute,std::abs(a.derivative-d));maximum(e.derivativeRelative,std::abs(a.derivative-d)/d);++e.points;};
 for(unsigned i=0;i<=500000;++i)check(-416+444.*i/500000);
 std::uint32_t rng=20931;for(unsigned i=0;i<100000;++i){rng=rng*1664525u+1013904223u;check(-416+444.*rng/4294967295.);}
 for(double z:{0.,-0.,1e-300,-1e-300,1e-16,-1e-16,-416.,28.,-40.,-36.}){check(z);check(std::nextafter(z,0.));}
 for(int k=-58;k<=40;++k)for(int j=-64;j<=64;++j){double z=k*std::log(2.)+j*std::log(2.)/128;if(z< -40||z>28)continue;check(z);const auto a=E::evaluate(std::nextafter(z,-INFINITY)),b=E::evaluate(std::nextafter(z,INFINITY));const double d=std::exp(z);maximum(e.boundaryValue,std::abs(a.value-b.value)/(2e-13*std::max(1.,d)));maximum(e.boundaryDerivative,std::abs(a.derivative-b.derivative)/(5e-16+2e-12*d));}
 for(double z:{double(NAN),double(INFINITY),double(-INFINITY),-417.,29.}){const auto a=E::evaluate(z);if(!std::isnan(a.value)||!std::isnan(a.derivative))++e.invalid;}
 if(!std::signbit(E::evaluate(-0.).value)||std::signbit(E::evaluate(0.).value))++e.invalid;
}
void compareDevice(const PerfApi* a,const GermaniumDevice& p,const std::vector<double>& ve,const std::vector<double>& vc,Errors& e){
 std::vector<PerfProbe> ref(ve.size()),fast(ve.size());perf_libm()->device(&p,sizeof(p),ve.data(),vc.data(),ref.data(),ve.size());a->device(&p,sizeof(p),ve.data(),vc.data(),fast.data(),ve.size());
 for(std::size_t i=0;i<ve.size();++i){if(ref[i].valid!=fast[i].valid){++e.invalid;continue;}if(!ref[i].valid)continue;for(unsigned j=0;j<6;++j){const double r=ref[i].value[j],v=fast[i].value[j];maximum(j<2?e.current:e.jacobian,std::abs(v-r)/(j<2?1e-12+1e-9*std::abs(r):1e-10+1e-8*std::abs(r)));}}
}
void devices(const PerfApi* a,Errors& e){
 std::vector<double> ve,vc;for(unsigned i=0;i<8192;++i){ve.push_back(-12+12.8*double((i*137u)%8192)/8191);vc.push_back(-12+12.8*double((i*1987u)%8192)/8191);}
 for(double x:{-.0,0.,.05,.15,.3,.5,.8,-12.,double(NAN),double(INFINITY),-12.01,.801})for(double y:{-12.,-9.,-.1,0.,.1,.8}){ve.push_back(x);vc.push_back(y);}
 for(unsigned mask=0;mask<34;++mask){GermaniumDevice p;if(mask<32){p.forwardGain=mask&1?300:40;p.reverseGain=mask&2?25:3;p.saturationAmps=mask&4?30e-6:10e-6;p.beLeakageAmps=mask&8?25e-6:0;p.bcLeakageAmps=mask&16?100e-6:0;}else if(mask==33)p.bcLeakageAmps*=10;compareDevice(a,p,ve,vc,e);}
}
void spiceDevices(const PerfApi* a,Errors& e){
 std::ifstream file(std::string(GERMANIUM_FIXTURES)+"/device-current.csv");
 if(!file)throw std::runtime_error("Missing independent device fixture");
 std::string row;std::getline(file,row);unsigned count=0;
 while(std::getline(file,row)){std::istringstream in(row);std::vector<std::string> fields;std::string f;while(std::getline(in,f,','))fields.push_back(f);
  if(fields.size()!=7)throw std::runtime_error("Invalid independent device fixture");
  GermaniumDevice p;p.forwardGain=std::stod(fields[1]);p.bcLeakageAmps=std::stod(fields[2]);double ve=std::stod(fields[3]),vc=std::stod(fields[4]);PerfProbe result{};
  a->device(&p,sizeof(p),&ve,&vc,&result,1);if(!result.valid)++e.invalid;
  for(unsigned j=0;j<2;++j){double expected=std::stod(fields[5+j]);maximum(e.spiceCurrent,std::abs(result.value[1-j]-expected)/(1e-10+1e-6*std::abs(expected)));}
  ++count;
 }
 if(count!=123)throw std::runtime_error("Incomplete independent device fixture");
}
struct Case {GermaniumCircuit c;float rate=48000,fuzz=.8f,volume=.5f;int mode=1;bool extreme=false;};
std::complex<double> projection(const std::vector<double>& x,unsigned bin){std::complex<double> s=0;for(unsigned i=0;i<x.size();++i)s+=x[i]*std::polar(1.,-2*pi*bin*i/x.size());return 2.*s/double(x.size());}
void circuit(const PerfApi* a,const Case& c,Errors& e,bool spectrum=false,unsigned bin=85){
 auto* ref=perf_libm();void* r=ref->create();void* f=a->create();bool ar=ref->configure(r,c.rate,c.fuzz,c.volume,c.mode,&c.c,sizeof(c.c)),af=a->configure(f,c.rate,c.fuzz,c.volume,c.mode,&c.c,sizeof(c.c));
 if(ar!=af)++e.configuration;
 if(!ar||!af){e.cases.push_back("{\"accepted\":false}");ref->destroy(r);a->destroy(f);return;}
 const auto rb=ref->state(r),fb=a->state(f);for(unsigned j=0;j<6;++j)maximum(e.bias,std::abs(rb.bias[j]-fb.bias[j])/2e-7);
 const unsigned n=spectrum?4096*5:8192;double sum=0,peak=0,nodePeak=0,inputPeak=0;std::uint64_t wr=0,wf=0,maxBaseR=0,maxBaseF=0;auto pr=rb,pf=fb;
 std::vector<double> yr,yf,ir,iff,source,ve1,vc1,ve2,vc2;
 for(unsigned i=0;i<n;++i){
  double x=.05*std::sin(2*pi*bin*i/4096);
  if(!spectrum){double env=i<1024?0:i<2048?(i-1024)/1024.:i<4096?1:i<5120?4:0;x=.05*env*(.8*std::sin(2*pi*997*i/c.rate)+.2*std::sin(2*pi*5993*i/c.rate));if(i>=5120&&i<6144)x=.05;if(c.extreme)x*=2.5;}
  float input=float(x),rout=0,fout=0;ref->block(r,&input,&rout,1);a->block(f,&input,&fout,1);auto rs=ref->state(r),fs=a->state(f);
  if(rs.failed-pr.failed!=fs.failed-pf.failed||rs.contained-pr.contained!=fs.contained-pf.contained)++e.events;
  const auto dr=rs.evals-pr.evals,df=fs.evals-pf.evals;maximum(e.work,double(dr>df?dr-df:df-dr)/8);wr+=dr;wf+=df;maxBaseR=std::max(maxBaseR,dr);maxBaseF=std::max(maxBaseF,df);pr=rs;pf=fs;
  double err=std::abs(double(rout)-fout);maximum(e.output,err/(c.extreme?2e-5:2e-6));sum+=err*err;peak=std::max(peak,err);
  for(unsigned j=0;j<6;++j){double ne=std::abs(rs.nodes[j]-fs.nodes[j]);maximum(e.nodes,ne/(c.extreme?2e-4:2e-5));nodePeak=std::max(nodePeak,ne);}
  maximum(e.input,std::abs(rs.nodes[6]-fs.nodes[6])/2e-8);inputPeak=std::max(inputPeak,std::abs(rs.nodes[6]-fs.nodes[6]));
  if(i%64==0){ve1.push_back(-rs.nodes[0]);vc1.push_back(rs.nodes[1]-rs.nodes[0]);ve2.push_back(rs.nodes[2]-rs.nodes[1]);vc2.push_back(rs.nodes[3]-rs.nodes[1]);}
  if(spectrum&&i>=n-4096){yr.push_back(rout);yf.push_back(fout);ir.push_back(rs.nodes[6]);iff.push_back(fs.nodes[6]);source.push_back(input);}
 }
 maximum(e.outputRms,std::sqrt(sum/n)/(c.extreme?2e-6:2e-7));maximum(e.work,double(wr>wf?wr-wf:wf-wr)/std::max(1.,.01*wr));
 compareDevice(a,c.c.q1,ve1,vc1,e);compareDevice(a,c.c.q2,ve2,vc2,e);
 if(spectrum){const auto fr=projection(yr,bin),ff=projection(yf,bin);double mag=std::abs(fr);maximum(e.fundamental,std::abs(mag-std::abs(ff))/(mag>1e-5?2e-4*mag:2e-7));
  for(unsigned h:{2,3,4,5})maximum(e.harmonic,std::abs(projection(yr,h*bin%4096)-projection(yf,h*bin%4096))/2e-6);
  auto residual=[](const std::vector<double>& x,double fundamental){double ss=0,mean=0;for(auto v:x){ss+=v*v;mean+=v;}mean/=x.size();return std::max(0.,ss/x.size()-mean*mean-.5*fundamental*fundamental);};double rr=residual(yr,mag),rf=residual(yf,std::abs(ff));
  if(rr>1e-12)maximum(e.thdn,std::abs(10*std::log10(std::max(rf,1e-30)/rr*std::norm(fr)/std::norm(ff)))/.05);
  if(c.mode==0){const auto zr=projection(source,bin)/projection(ir,bin)-c.c.sourceOhms,zf=projection(source,bin)/projection(iff,bin)-c.c.sourceOhms;maximum(e.impedance,std::abs(zf-zr)/(1e-4*std::abs(zr)));}
 }

 const auto rs=ref->state(r),fs=a->state(f);
 std::ostringstream record;record<<std::setprecision(12)<<"{\"rate\":"<<c.rate<<",\"fuzz\":"<<c.fuzz<<",\"volume\":"<<c.volume<<",\"mode\":"<<c.mode<<",\"source_ohms\":"<<c.c.sourceOhms<<",\"supply_volts\":"<<c.c.supplyVolts<<",\"feedback_ohms\":"<<c.c.feedbackOhms<<",\"q1_bc_leakage\":"<<c.c.q1.bcLeakageAmps<<",\"spectrum_bin\":"<<(spectrum?bin:0)<<",\"extreme\":"<<(c.extreme?"true":"false")<<",\"output_peak_volts\":"<<peak<<",\"output_rms_volts\":"<<std::sqrt(sum/n)<<",\"nodes_peak_volts\":"<<nodePeak<<",\"input_peak_amps\":"<<inputPeak<<",\"reference_evaluations\":"<<wr<<",\"candidate_evaluations\":"<<wf<<",\"max_base_reference_evaluations\":"<<maxBaseR<<",\"max_base_candidate_evaluations\":"<<maxBaseF<<",\"reference_fallbacks\":"<<rs.failed<<",\"candidate_fallbacks\":"<<fs.failed<<",\"reference_containments\":"<<rs.contained<<",\"candidate_containments\":"<<fs.contained<<"}";
 e.cases.push_back(record.str());e.frames+=n;ref->destroy(r);a->destroy(f);
}
std::vector<Case> cases(){std::vector<Case> out;
 for(unsigned fault=0;fault<3;++fault)for(unsigned condition=0;condition<4;++condition){Case c;if(fault==1)c.c.q1.bcLeakageAmps*=10;if(fault==2)c.c.feedbackOhms=10000;if(condition==1)c.c.sourceOhms=100000;if(condition==2){c.fuzz=1;c.extreme=true;}if(condition==3){c.mode=0;c.extreme=true;}out.push_back(c);}
 struct L{double GermaniumCircuit::*p;double lo,hi;};for(auto l:{L{&GermaniumCircuit::supplyVolts,6,9},L{&GermaniumCircuit::sourceOhms,100,100000},L{&GermaniumCircuit::inputFarads,1e-6,4.7e-6},L{&GermaniumCircuit::collector1Ohms,20000,50000},L{&GermaniumCircuit::feedbackOhms,10000,150000},L{&GermaniumCircuit::fuzzOhms,500,2000},L{&GermaniumCircuit::bypassFarads,10e-6,47e-6},L{&GermaniumCircuit::collector2Ohms,4700,22000},L{&GermaniumCircuit::supplyTapOhms,220,1000},L{&GermaniumCircuit::outputFarads,5e-9,22e-9},L{&GermaniumCircuit::volumeOhms,250000,1000000},L{&GermaniumCircuit::loadOhms,100000,2000000}})for(double v:{l.lo,l.hi}){Case c;c.c.*l.p=v;c.extreme=true;out.push_back(c);}
 struct D{double GermaniumDevice::*p;double lo,hi;};for(auto d:{D{&GermaniumDevice::forwardGain,40,300},D{&GermaniumDevice::reverseGain,3,25},D{&GermaniumDevice::saturationAmps,10e-6,30e-6},D{&GermaniumDevice::beLeakageAmps,0,25e-6},D{&GermaniumDevice::bcLeakageAmps,0,100e-6}})for(bool second:{false,true})for(double v:{d.lo,d.hi}){Case c;(second?c.c.q2:c.c.q1).*d.p=v;c.extreme=true;out.push_back(c);}
 for(float rate:{32000.f,44100.f,96000.f})for(float fuzz:{0.f,1.f}){Case c;c.rate=rate;c.fuzz=fuzz;c.extreme=true;out.push_back(c);}
 for(float vol:{0.f,1.f}){Case c;c.volume=vol;c.extreme=true;out.push_back(c);}
 for(double rail:{6.,9.})for(double source:{100.,100000.}){Case c;c.c.supplyVolts=rail;c.c.sourceOhms=source;c.c.q1.forwardGain=40;c.c.q2.forwardGain=300;c.c.q1.bcLeakageAmps=100e-6;c.c.q2.beLeakageAmps=25e-6;c.extreme=true;out.push_back(c);}
 return out;
}
bool passes(const Errors& e){return std::max({e.value,e.derivative,e.boundaryValue,e.boundaryDerivative,e.current,e.jacobian,e.spiceCurrent,e.output,e.outputRms,e.nodes,e.bias,e.input,e.fundamental,e.harmonic,e.thdn,e.impedance,e.work})<=1&&!(e.events+e.configuration+e.invalid);}
void print(const PerfApi* a,const Errors& e,unsigned count){
 std::cout<<"{\"implementation\":\""<<a->name<<"\",\"passes_all_gates\":"<<(passes(e)?"true":"false")<<",\"exponential_points\":"<<e.points<<",\"circuit_cases\":"<<count<<",\"frames\":"<<e.frames<<",\"value_absolute\":"<<e.valueAbsolute<<",\"derivative_absolute_stress\":"<<e.derivativeAbsolute<<",\"derivative_relative_including_tail\":"<<e.derivativeRelative<<",\"gate_ratios\":{";
 bool comma=false;auto field=[&](const char* name,double v){if(comma)std::cout<<',';comma=true;std::cout<<'\"'<<name<<"\":";if(std::isfinite(v))std::cout<<v;else std::cout<<"null";};
 field("function",e.value);field("derivative",e.derivative);field("boundary_value",e.boundaryValue);field("boundary_derivative",e.boundaryDerivative);field("currents",e.current);field("jacobian",e.jacobian);field("independent_device",e.spiceCurrent);field("raw_output_peak",e.output);field("raw_output_rms",e.outputRms);field("nodes",e.nodes);field("dc_bias",e.bias);field("input_current",e.input);field("fundamental",e.fundamental);field("harmonics",e.harmonic);field("thdn",e.thdn);field("impedance",e.impedance);field("solver_work",e.work);
 std::cout<<"},\"event_mismatches\":"<<e.events<<",\"configuration_mismatches\":"<<e.configuration<<",\"invalid_mismatches\":"<<e.invalid<<",\"cases\":[";for(unsigned i=0;i<e.cases.size();++i){if(i)std::cout<<',';std::cout<<e.cases[i];}std::cout<<"]}";
}
int main(int argc,char** argv){std::cout<<std::setprecision(12)<<"{\"schema\":1,\"gate_document\":\"docs/GERMANIUM_PERF_PLAN.md\",\"candidates\":[";const auto matrix=cases();unsigned index=0;bool required=true;int matches=0;
 for(const auto* a:{perf_h32(),perf_h64(),perf_p8(),perf_p12()}){Errors e;if(index)std::cout<<',';if(index==0)exponential<Hermite32>(e);if(index==1)exponential<Hermite64>(e);if(index==2)exponential<Polynomial8>(e);if(index==3)exponential<Polynomial12>(e);devices(a,e);spiceDevices(a,e);
 unsigned count=0;if(e.value<=1&&e.derivative<=1&&e.current<=1&&e.jacobian<=1){for(const auto& c:matrix){circuit(a,c,e);++count;}for(unsigned fault=0;fault<3;++fault)for(unsigned bin:{9,85,511})for(int mode:{0,1}){Case c;if(fault==1)c.c.q1.bcLeakageAmps*=10;if(fault==2)c.c.feedbackOhms=10000;c.mode=mode;circuit(a,c,e,true,bin);++count;}}
 print(a,e,count);for(int argument=1;argument<argc;++argument)if(std::string(argv[argument])==a->name){required=required&&passes(e);++matches;}++index;
 }std::cout<<"]}\n";return required&&matches==argc-1?0:1;}
