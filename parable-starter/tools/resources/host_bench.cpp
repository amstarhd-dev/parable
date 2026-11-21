#include "adapters.h"
#include "host_tracker.h"
#include "lab_wav.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>
namespace resource_bench {using Selected=BENCH_SELECTED;}
using resource_bench::Selected;
constexpr unsigned block=48;
static std::vector<float> excitation(unsigned frames,const std::string& name){
 std::vector<float> v(frames);uint32_t random=0x504152;
 if(name=="guitar"){auto w=lab::read(BENCH_GUITAR_PATH);if(w.rate!=48000)throw std::runtime_error("Guitar must be48kHz");float peak=0;for(float x:w.samples)peak=std::max(peak,std::abs(x));if(!peak)throw std::runtime_error("Silent guitar");for(unsigned n=0;n<frames;++n)v[n]=.2f*w.samples[n%w.samples.size()]/peak;return v;}
 for(unsigned n=0;n<frames;++n){double t=double(n)/48000;random=1664525*random+1013904223;
  float noise=float(int32_t(random))/2147483648.f;
  if(name=="silence")v[n]=0;
  else if(name=="pluck")v[n]=float(.15*std::exp(-14*std::fmod(t,.09))*(std::sin(2*3.141592653589793*110*t)+.25*std::sin(2*3.141592653589793*220*t)));
  else if(name=="high_note")v[n]=float(.2*std::sin(2*3.141592653589793*7971*t));
  else if(name=="noise")v[n]=.2f*noise;
  else if(name=="hard_edges")v[n]=(n/17)%2?.9f:-.9f;
  else if(name=="burst_tail")v[n]=n<frames/4?.2f*noise:0;
  else if(name=="dc")v[n]=n<frames/2?.8f:-.8f;
  else if(name=="invalid")v[n]=n%97==0?NAN:n%101==0?INFINITY:n%103==0?-INFINITY:.1f*noise;
  else if(name=="extreme")v[n]=n%5==0?std::numeric_limits<float>::max():n%5==1?-std::numeric_limits<float>::max():.2f*noise;
  else if(name=="denormal")v[n]=(n%2?1:-1)*std::numeric_limits<float>::denorm_min();
  else throw std::runtime_error("Unknown stimulus");
 }return v;
}
// One actual48-frame callback. No output hash/statistics are inside its timer.
__attribute__((noinline)) static void benchBlock(Selected& model,const float* input,float* output){for(unsigned n=0;n<block;++n)output[n]=model.process(input[n]);}
int main(int argc,char** argv){try{
 if(argc!=6)throw std::runtime_error("usage: BENCH stimulus mode frames repetitions route");
 const std::string signal=argv[1];unsigned mode=std::stoul(argv[2]),frames=std::stoul(argv[3]),repeats=std::stoul(argv[4]),route=std::stoul(argv[5]);
 if(mode>4||route>2||frames<48||frames>480000||frames%block||repeats<2||repeats>10)throw std::runtime_error("Invalid bounded benchmark configuration");
 auto input=excitation(frames,signal);std::vector<float> output(frames);std::vector<double> times;times.reserve(size_t(frames/block)*repeats);
 uint64_t firstHash=0,nonfinite=0;bool deterministic=true,prepared=true;double initNs=0,reprepareNs=0,peak=0,rms=0;uint64_t repreparePeak=0;bool reprepareOk=true;uint64_t live=0,allocPeak=0,allocations=0,forbidden=0,overflows=0;resource_bench::Counters counters{};
 for(unsigned run=0;run<repeats;++run){
  resource_bench::tracking::start();auto initStart=std::chrono::steady_clock::now();auto model=std::make_unique<Selected>();
#ifdef BENCH_ROUTE
  model->route=int(route);
#else
  (void)route;
#endif
#ifdef BENCH_UTILITY_KIND
  model->kind=BENCH_UTILITY_KIND;
#endif
  prepared=model->prepare(48000,int(mode));initNs+=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-initStart).count();
  auto& m=resource_bench::tracking::metrics;live=std::max(live,m.live);allocPeak=std::max(allocPeak,m.peak);allocations=std::max(allocations,m.allocations);
  resource_bench::tracking::enabled=false;if(!prepared)break;
  for(unsigned n=0;n<frames;n+=block){
   auto begin=std::chrono::steady_clock::now();resource_bench::tracking::audio=true;
   benchBlock(*model,input.data()+n,output.data()+n);resource_bench::tracking::audio=false;
   times.push_back(std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-begin).count());
  }
  auto c=model->counters();counters.failures=std::max(counters.failures,c.failures);counters.evaluations=std::max(counters.evaluations,c.evaluations);counters.solves=std::max(counters.solves,c.solves);counters.max_iterations=std::max(counters.max_iterations,c.max_iterations);counters.containment=std::max(counters.containment,c.containment);
  forbidden+=m.callbackAllocations+m.callbackFrees+m.locks+m.io;overflows+=m.overflow;
  uint64_t hash=14695981039346656037ULL;double power=0;
  for(float x:output){if(!std::isfinite(x)){++nonfinite;continue;}peak=std::max(peak,double(std::abs(x)));power+=double(x)*x;uint32_t bits;std::memcpy(&bits,&x,4);for(unsigned b=0;b<4;++b){hash^=(bits>>(8*b))&255;hash*=1099511628211ULL;}}
  rms=std::max(rms,std::sqrt(power/frames));if(!run)firstHash=hash;else deterministic=deterministic&&hash==firstHash;
  resource_bench::tracking::enabled=true;auto again=std::chrono::steady_clock::now();reprepareOk=reprepareOk&&model->prepare(48000,int(mode));reprepareNs+=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-again).count();repreparePeak=std::max(repreparePeak,m.peak);resource_bench::tracking::enabled=false;
 }
 double mean=0;for(double t:times)mean+=t;if(!times.empty())mean/=times.size();std::sort(times.begin(),times.end());
 auto quantile=[&](double q){return times.empty()?0:times[std::min(times.size()-1,size_t(q*(times.size()-1)))];};
 std::cout<<std::setprecision(12)<<"{\"prepared\":"<<(prepared?"true":"false")<<",\"deterministic\":"<<(deterministic?"true":"false")<<",\"frames\":"<<frames<<",\"repetitions\":"<<repeats<<",\"callbacks\":"<<times.size()<<",\"object_bytes\":"<<sizeof(Selected)<<",\"live_requested_heap_including_object_bytes\":"<<live<<",\"peak_prepare_requested_heap_bytes\":"<<allocPeak<<",\"preparation_allocations\":"<<allocations<<",\"initialization_mean_ns_includes_tracking\":"<<initNs/repeats<<",\"reprepare_mean_ns_includes_tracking\":"<<reprepareNs/repeats<<",\"reprepare_peak_requested_heap_bytes\":"<<repreparePeak<<",\"reprepare_ok\":"<<(reprepareOk?"true":"false")<<",\"callback_mean_ns\":"<<mean<<",\"callback_p50_ns\":"<<quantile(.5)<<",\"callback_p95_ns\":"<<quantile(.95)<<",\"callback_max_ns\":"<<quantile(1)<<",\"mean_ns_frame\":"<<mean/block<<",\"nonfinite_outputs\":"<<nonfinite<<",\"forbidden_callback_operations\":"<<forbidden<<",\"allocation_tracker_overflows\":"<<overflows<<",\"failures\":"<<counters.failures<<",\"evaluations\":"<<counters.evaluations<<",\"solves\":"<<counters.solves<<",\"max_iterations\":"<<counters.max_iterations<<",\"containment\":"<<counters.containment<<",\"peak\":"<<peak<<",\"rms\":"<<rms<<",\"output_fnv1a64\":\""<<std::hex<<firstHash<<"\"}\n";
 return 0;
 }catch(const std::exception& e){resource_bench::tracking::audio=false;resource_bench::tracking::enabled=false;std::cerr<<e.what()<<'\n';return 1;}}
