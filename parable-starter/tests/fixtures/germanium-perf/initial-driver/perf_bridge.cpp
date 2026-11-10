#include "perf_bridge.h"
#include "parable_germanium.h"
#include "germanium_device.h"
#include <cstring>
#include <type_traits>
namespace {
using namespace parable::dsp;
void* create(){return new Germanium;}
void destroy(void* p){delete static_cast<Germanium*>(p);}
bool configure(void* p,float rate,float fuzz,float volume,int mode,const void* bytes,std::size_t size){
 if(size!=sizeof(GermaniumCircuit))return false;GermaniumCircuit c;std::memcpy(&c,bytes,size);
 return static_cast<Germanium*>(p)->configure(rate,{fuzz,volume},c,static_cast<GermaniumRate>(mode));
}
void reset(void* p){static_cast<Germanium*>(p)->reset();}
void block(void* p,const float* in,float* out,std::size_t n){static_cast<Germanium*>(p)->processBlock(in,out,n);}
PerfState state(const void* p){const auto& m=*static_cast<const Germanium*>(p);const auto& n=m.lastNodes();const auto& b=m.bias();
 return {{n.base1,n.collector1,n.emitter2,n.collector2,n.tap,n.jack,n.inputAmps,n.residualAmps},
 {b.base1,b.collector1,b.emitter2,b.collector2,b.tap,b.jack},m.solverCalls(),m.loopEvaluations(),m.failedSolves(),m.containedOutputs(),m.maximumLoopEvaluations(),n.iterations};}
void device(const void* bytes,std::size_t size,const double* ve,const double* vc,PerfProbe* out,std::size_t n){
 if(size!=sizeof(GermaniumDevice))return;GermaniumDevice p;std::memcpy(&p,bytes,size);
 for(std::size_t i=0;i<n;++i){const auto x=germanium_detail::device(ve[i],vc[i],p);out[i]={{x.collector,x.base,x.collectorE,x.collectorC,x.baseE,x.baseC},x.valid};}
}
const PerfApi api{"libm",create,destroy,configure,reset,block,state,device};
}
extern "C" const PerfApi* perf_libm(){return &api;}
