#include "perf_bridge.h"
#include "parable_germanium.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>
#ifdef PERF_COUNT
std::uint64_t expCalls=0,sqrtCalls=0;
extern "C" double __real_expm1(double);extern "C" double __real_sqrt(double);
extern "C" double __wrap_expm1(double x){++expCalls;return __real_expm1(x);}
extern "C" double __wrap_sqrt(double x){++sqrtCalls;return __real_sqrt(x);}
#else
std::uint64_t expCalls=0,sqrtCalls=0;
#endif
volatile double sink=0;
using Clock=std::chrono::steady_clock;
using namespace parable::dsp;
struct Case {int fault,condition;};
int main(int argc,char** argv){try{
 const unsigned trials=argc>1?unsigned(std::stoul(argv[1])):7,repeats=argc>2?unsigned(std::stoul(argv[2])):128;
 if(trials<3||repeats<1||trials>31||repeats>1024)throw std::runtime_error("Invalid trials/repeats");
 constexpr unsigned n=4096,blockSize=64;constexpr double pi=3.14159265358979323846;
 const std::vector<const PerfApi*> implementations{perf_libm()};
 std::cout<<std::setprecision(12)<<"{\"frames_per_trial\":"<<n*repeats<<",\"warmup_frames\":"<<2*n<<",\"block_frames\":64,\"trials\":"<<trials<<",\"instrumented\":"
#ifdef PERF_COUNT
 <<"true"
#else
 <<"false"
#endif
 <<",\"samples\":[\n";bool comma=false;
 auto row=[&](const char* kind,const char* implementation,int caseId,unsigned trial,double ns,double sum,const PerfState& before,const PerfState& after,std::uint64_t ex,std::uint64_t sq){
  if(comma)std::cout<<",\n";comma=true;
  std::cout<<"{\"kind\":\""<<kind<<"\",\"implementation\":\""<<implementation<<"\",\"case\":"<<caseId<<",\"trial\":"<<trial<<",\"ns_per_sample\":"<<ns
   <<",\"checksum\":"<<sum<<",\"solver_evaluations\":"<<after.evals-before.evals<<",\"failed\":"<<after.failed-before.failed<<",\"contained\":"<<after.contained-before.contained
   <<",\"max_evaluations\":"<<after.maximum<<",\"expm1_calls\":"<<ex<<",\"sqrt_library_calls\":"<<sq<<"}";
 };
 std::array<float,n> input{},output{};
 for(unsigned trial=0;trial<trials;++trial)for(int id=0;id<12;++id)for(std::size_t v=0;v<implementations.size();++v){const auto* a=implementations[(v+trial)%implementations.size()];
  const int fault=id/4,condition=id%4;GermaniumCircuit c;if(fault==1)c.q1.bcLeakageAmps*=10;if(fault==2)c.feedbackOhms=10000;
  const double level=condition==0?.005:condition==3?.2:.05;c.sourceOhms=condition==2?50000:condition==3?100:1000;
  for(unsigned i=0;i<n;++i)input[i]=float(level*(.8*std::sin(2*pi*85*i/n)+.2*std::sin(2*pi*511*i/n)));
  void* m=a->create();if(!a->configure(m,48000,condition==3?1.f:.8f,.5f,1,&c,sizeof(c)))throw std::runtime_error("Benchmark configure failed");
  for(int warm=0;warm<2;++warm)for(unsigned i=0;i<n;i+=blockSize)a->block(m,input.data()+i,output.data()+i,blockSize);
  const auto before=a->state(m);expCalls=sqrtCalls=0;const auto begin=Clock::now();
  for(unsigned repeat=0;repeat<repeats;++repeat)for(unsigned i=0;i<n;i+=blockSize)a->block(m,input.data()+i,output.data()+i,blockSize);
  const auto end=Clock::now();const auto ex=expCalls,sq=sqrtCalls;const auto after=a->state(m);double sum=0;for(float x:output)sum+=x;sink=sum;
  const double ns=std::chrono::duration<double,std::nano>(end-begin).count()/(n*repeats);
  row("processor",a->name,id,trial,ns,sum,before,after,ex,sq);a->destroy(m);
 }
 std::array<double,n> ve{},vc{};std::array<PerfProbe,n> result{};
 for(unsigned trial=0;trial<trials;++trial)for(int id=0;id<6;++id)for(std::size_t v=0;v<implementations.size();++v){const auto* a=implementations[(v+trial)%implementations.size()];
  GermaniumDevice p;if(id/2==1)p.bcLeakageAmps*=10;if(id/2==2){p.forwardGain=40;p.reverseGain=3;p.saturationAmps=30e-6;p.beLeakageAmps=25e-6;p.bcLeakageAmps=100e-6;}
  std::uint32_t rng=0x00320;for(unsigned i=0;i<n;++i){if(id%2==0){ve[i]=.1+.1*std::sin(i*.119);vc[i]=-4.5-4.4*std::sin(i*.07);}
   else{rng=rng*1664525u+1013904223u;ve[i]=-12+12.8*double(rng)/4294967295.;rng=rng*1664525u+1013904223u;vc[i]=-12+12.8*double(rng)/4294967295.;}}
  a->device(&p,sizeof(p),ve.data(),vc.data(),result.data(),n);expCalls=sqrtCalls=0;const auto begin=Clock::now();
  for(unsigned repeat=0;repeat<repeats;++repeat)a->device(&p,sizeof(p),ve.data(),vc.data(),result.data(),n);
  const auto end=Clock::now();const auto ex=expCalls,sq=sqrtCalls;double sum=0;for(const auto& x:result)for(double y:x.value)sum+=y;sink=sum;
  row("device",a->name,id,trial,std::chrono::duration<double,std::nano>(end-begin).count()/(n*repeats),sum,{}, {},ex,sq);
 }
 std::cout<<"\n]}\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
