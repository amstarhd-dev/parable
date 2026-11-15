// Offline reproducible solver/table/sampling alternatives, NOT an audio API.
#include "parable_wavefolder.h"
#include "wavefolder_cell.h"
#include "lab_wav.h"
#include <chrono>
#include <iomanip>
#include <iostream>
using parable::dsp::Wavefolder;namespace cell=parable::dsp::wavefolder_detail;
constexpr double pi=3.14159265358979323846;
int main(int argc,char** argv)try{
 lab::require(argc==2,"wavefolder_study OUTPUT_DIR");const std::filesystem::path out=argv[1];std::filesystem::create_directories(out);std::ofstream cost(out/"cost.csv"),direct(out/"direct.csv"),knots(out/"knots.csv");for(auto* f:{&cost,&direct,&knots})*f<<std::setprecision(17);
 direct<<"Rplus,max_updates,failed,wall_us_per_call\n";knots<<"Rplus,intervals,input_V,readout_V,derivative\n";cost<<"rate,fault,kind,factor,ADAA,wall_us_per_frame,state_bytes\n";volatile double sink=0;
 for(double rp:{15000.,22000.}){
  for(unsigned maximum:{4u,8u,16u,64u}){unsigned failed=0;const auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<=6000;++i){const auto p=cell::solve(-3+i*.001,rp,10000,15,maximum);if(!p.valid)++failed;else sink+=p.value;}direct<<rp<<','<<maximum<<','<<failed<<','<<std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/6001<<'\n';}
  for(unsigned count:{256u,512u,1024u}){cell::Vec previous{};bool have=false;for(unsigned i=0;i<=count;++i){const auto p=cell::solve(-3+6.*i/count,rp,10000,15,64,have?&previous:nullptr);lab::require(p.valid,"Knot solve failed");previous=p.nodes;have=true;knots<<rp<<','<<count<<','<<-3+6.*i/count<<','<<float(p.value)<<','<<float(p.slope)<<'\n';}}
 }
 constexpr unsigned n=4096;
 for(unsigned rate:{44100u,48000u,96000u})for(bool fault:{false,true})for(unsigned kind=0;kind<4;++kind){const unsigned bin=(kind==0?1760:kind==1?3520:kind==2?8000:1000)*n/rate;std::vector<float>x(n),y(n);double peak=0;
  for(unsigned i=0;i<n;++i){double a=0;if(kind<3)a=std::sin(2*pi*bin*i/n);else for(unsigned b:{76u,97u,113u})for(unsigned h=1;h<=5;++h)a+=std::sin(2*pi*b*h*i/n)/(h*h);peak=std::max(peak,std::abs(a));x[i]=float(a);}for(auto& a:x)a=float(a*.2/peak);
  for(unsigned factor:{1u,2u,4u})for(bool aa:{false,true}){Wavefolder m;lab::require(m.prepare(float(rate),{6,fault},factor,aa),"Prepare failed");const auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<8*n;++i){const auto a=m.process(x[i%n]);if(i>=7*n)y[i%n]=a;}const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/(8*n);lab::require(!m.failures()&&!m.tableClamps()&&!m.outputClamps(),"Containment in study");cost<<rate<<','<<fault<<','<<kind<<','<<factor<<','<<aa<<','<<us<<','<<sizeof(m)<<'\n';
   std::ofstream f(out/("capture-"+std::to_string(rate)+"-"+std::to_string(fault)+"-"+std::to_string(kind)+"-"+std::to_string(factor)+"-"+std::to_string(aa)+".csv"));f<<"input_V,native_normalized\n"<<std::setprecision(17);for(unsigned i=0;i<n;++i)f<<x[i]<<','<<y[i]<<'\n';lab::require(bool(f),"Capture write failed");}
 }
 lab::require(bool(cost)&&bool(direct)&&bool(knots),"Study write failed");return sink==1e300;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
