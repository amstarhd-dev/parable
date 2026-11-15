// Offline full-processor sampling/cost comparison. No production integration.
#include "parable_fz1.h"
#include "lab_wav.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
using parable::dsp::Fz1;using parable::dsp::Fz1Config;
constexpr double pi=3.14159265358979323846;
constexpr unsigned n=4096;
static double excitation(unsigned i,unsigned rate,unsigned kind,unsigned up=1){const double t=double(i)/(n*up);double y=0;if(kind<3){const unsigned bin=(kind==0?1760:kind==1?3520:8000)*n/rate;y=std::sin(2*pi*bin*t);}else for(unsigned b:{9u,12u,15u})for(unsigned h=1;h<=5;++h)y+=std::sin(2*pi*b*h*t)/(h*h);return .2*y/(kind<3?1:4.390833333333333);}
int main(int argc,char** argv)try{lab::require(argc==2||(argc==3&&std::string(argv[2])=="--reference-only"),"fz1_study OUTPUT_DIR [--reference-only]");const std::filesystem::path out=argv[1];std::filesystem::create_directories(out);std::ofstream cost(out/(argc==3?"reference-cost.csv":"cost.csv"));cost<<std::setprecision(17)<<"rate,fault,kind,factor,wall_us_per_frame,CPU_us_per_frame,max_evaluations,mean_evaluations,state_bytes,failures\n";
 for(unsigned rate:{44100u,48000u,96000u})for(bool fault:{false,true})for(unsigned kind=0;kind<4;++kind){Fz1Config c;c.attack=1;c.openBiasFeed=fault;
  //16x exact native GP with continuous analytic input, then independent ideal
  //bandlimiting/decimation in Python. SPICE separately checks circuit dynamics.
  for(unsigned factor:{1u,2u,4u,8u,0u}){if(argc==3&&factor!=0)continue;Fz1 m;const bool reference=factor==0;lab::require(m.prepare(float(reference?rate*16:rate),c,reference?1:factor),"Prepare study failed");const unsigned up=reference?16:1,warm=8*n*up,total=warm+n*up;std::vector<float> y(n*up);const auto cpu=std::clock();const auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<total;++i){const auto v=m.process(float(excitation(i,rate,kind,up)));if(i>=warm)y[i-warm]=v;}const double wall=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/total,cpuUs=1e6*double(std::clock()-cpu)/CLOCKS_PER_SEC/total;
   /* Rejected sampling strategies retain counted failures and captures. */cost<<rate<<','<<fault<<','<<kind<<','<<factor<<','<<wall<<','<<cpuUs<<','<<m.maximumEvaluations()<<','<<double(m.evaluations())/m.solves()<<','<<sizeof(m)<<','<<m.failures()<<'\n';
   std::ofstream f(out/("capture-"+std::to_string(rate)+"-"+std::to_string(fault)+"-"+std::to_string(kind)+"-"+std::to_string(factor)+".csv"));f<<"native_normalized\n"<<std::setprecision(17);for(float v:y)f<<v<<'\n';lab::require(bool(f),"Capture write failed");}
 }
 lab::require(bool(cost),"Cost receipt write failed");return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
