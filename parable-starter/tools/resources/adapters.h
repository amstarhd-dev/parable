#pragma once
// Laboratory adapters only. Original processors are compiled from pinned Git blobs.
#include <array>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cmath>
namespace resource_bench {
struct Counters {uint64_t failures=0,evaluations=0,solves=0,max_iterations=0,containment=0;};
#define OPTIONAL_COUNTER(name) template<class T> auto get_##name(const T& x,int)->decltype(x.name()){return x.name();} template<class T> uint64_t get_##name(const T&,long){return 0;}
OPTIONAL_COUNTER(failures) OPTIONAL_COUNTER(failedSolves) OPTIONAL_COUNTER(numericalFaults)
OPTIONAL_COUNTER(evaluations) OPTIONAL_COUNTER(loopEvaluations) OPTIONAL_COUNTER(solves) OPTIONAL_COUNTER(solverCalls)
OPTIONAL_COUNTER(maximumEvaluations) OPTIONAL_COUNTER(maximumLoopEvaluations)
OPTIONAL_COUNTER(outputClamps) OPTIONAL_COUNTER(containmentEvents) OPTIONAL_COUNTER(nonfiniteOutputs)
#undef OPTIONAL_COUNTER
#ifdef BENCH_ARM
float* reserveSamples(size_t n) noexcept;
struct Storage {float* pointer=nullptr;size_t count=0;void resize(size_t n){pointer=reserveSamples(n);count=pointer?n:0;}float* data(){return pointer;}size_t size()const{return count;}};
#else
using Storage=std::vector<float>;
#endif
template<class T> struct Holder {
 T fx;
 Counters counters()const {return {uint64_t(get_failures(fx,0))+get_failedSolves(fx,0)+get_numericalFaults(fx,0),
 uint64_t(get_evaluations(fx,0))+get_loopEvaluations(fx,0),uint64_t(get_solves(fx,0))+get_solverCalls(fx,0),
 std::max(uint64_t(get_maximumEvaluations(fx,0)),uint64_t(get_maximumLoopEvaluations(fx,0))),
 uint64_t(get_outputClamps(fx,0))+get_containmentEvents(fx,0)+get_nonfiniteOutputs(fx,0)};}
};
}
#ifdef BENCH_USE_RAT
#include "parable_rat.h"
namespace resource_bench {struct Rat:Holder<parable::dsp::Rat> {
 bool prepare(float r,int m){parable::dsp::RatCircuit c;if(m==1)c.positiveDiodeConnected=false;if(m==3)c.trebleLegConnected=false;if(m==4)c.feedbackCompFarads=1e-7;
 #ifdef PARABLE_RAT_BENCH_1X
 return fx.configure(r,{m==2?1.f:.65f,.5f,.75f},c,parable::dsp::RatRate::Reference1x);
#else
 return fx.configure(r,{m==2?1.f:.65f,.5f,.75f},c);
#endif
 }
 float process(float x){return fx.process(x);}
};}
#endif
#ifdef BENCH_USE_CMOS
#include "parable_cmos.h"
namespace resource_bench {struct Cmos:Holder<parable::dsp::Cmos>{
 bool prepare(float r,int m){parable::dsp::CmosCircuit c;
#ifdef BENCH_GEMINI
 c=parable::dsp::geminiCircuit(m==1);
#else
 if(m==1)c.interstageFarads=330e-12;if(m==3)c.secondFeedbackOhms=100e3;
#endif
 return fx.configure(r,{m==2?1.f:.6f,.5f},c);}
 float process(float x){return fx.process(x);}
};}
#endif
#ifdef BENCH_USE_GERMANIUM
#include "parable_germanium.h"
namespace resource_bench {struct Germanium:Holder<parable::dsp::Germanium>{
 bool prepare(float r,int m){parable::dsp::GermaniumCircuit c;if(m==1)c.q1.bcLeakageAmps*=10;if(m==3)c.feedbackOhms=10e3;
 return fx.configure(r,{m==2?1.f:.8f,.5f},c);}
 float process(float x){return fx.process(x);}
};}
#endif
#ifdef BENCH_USE_MUFF
#include "parable_muff.h"
namespace resource_bench {struct Muff:Holder<parable::dsp::Muff>{bool prepare(float r,int m){return fx.prepare(r,{m==1,m==2?1.f:.75f,.5f,.5f});}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_TS808
#include "parable_ts808.h"
namespace resource_bench {struct Ts808:Holder<parable::dsp::Ts808>{bool prepare(float r,int m){return fx.prepare(r,{m==2?1.f:.5f,.5f,.5f,m==1});}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_KLON
#include "parable_klon.h"
namespace resource_bench {struct Klon:Holder<parable::dsp::Klon>{bool prepare(float r,int m){return fx.prepare(r,{m==2?1.f:.5f,.5f,.5f,m==1});}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_WAVEFOLDER
#include "parable_wavefolder.h"
namespace resource_bench {struct Wavefolder:Holder<parable::dsp::Wavefolder>{bool prepare(float r,int m){return fx.prepare(r,{m==2?6.f:2.f,m==1});}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_SCORPIO
#include "parable_scorpio.h"
namespace resource_bench {struct Scorpio:Holder<parable::dsp::Scorpio>{bool prepare(float r,int m){return fx.prepare(r,{m==2?1.f:.5f,.35f,.8f,m==1});}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_LIBRA
#include "parable_libra.h"
namespace resource_bench {struct Libra:Holder<parable::dsp::Libra>{bool prepare(float r,int m){return fx.prepare(r,{m==2?1.f:.5f,.8f,m==1});}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_DS1
#include "parable_capricorn.h"
namespace resource_bench {struct Ds1:Holder<parable::dsp::Capricorn>{bool prepare(float r,int m){return fx.prepare(r,{m==2?1.f:.5f,.5f,.8f,m==1});}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_FZ1
#include "parable_fz1.h"
namespace resource_bench {struct Fz1:Holder<parable::dsp::Fz1>{bool prepare(float r,int m){parable::dsp::Fz1Config c;c.openBiasFeed=m==1;c.attack=m==2?1.f:.5f;return fx.prepare(r,c);}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_BLENDER
#include "parable_blender.h"
namespace resource_bench {struct Blender:Holder<parable::dsp::Blender>{bool prepare(float r,int m){parable::dsp::BlenderConfig c;c.d1Open=m==1;c.boost=m==2;c.sustain=m==2?1.f:.5f;return fx.prepare(r,c);}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_TAPE_SAT
#include "parable_tape_saturation.h"
namespace resource_bench {struct TapeSat:Holder<parable::dsp::TapeSaturation>{bool prepare(float r,int m){return fx.prepare(r,m==1);}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_TAPE
#include "parable_tape.h"
namespace resource_bench {struct Tape:Holder<parable::dsp::Tape>{Storage storage;bool prepare(float r,int m){parable::dsp::TapeConfig c;c.eraseRetention=m==1?.85f:0;c.feedback=m==2?.8f:.5f;storage.resize(fx.requiredCells(r,c));return fx.prepare(r,c,storage.data(),storage.size());}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_ROTARY
#include "parable_rotary.h"
namespace resource_bench {struct Rotary:Holder<parable::dsp::Rotary>{bool prepare(float r,int m){return fx.prepare(r,{m==1,.65f});}float process(float x){return fx.process(x,.8f);}};}
#endif
#ifdef BENCH_USE_SPRING
#include "parable_spring.h"
namespace resource_bench {struct Spring:Holder<parable::dsp::Spring>{bool prepare(float r,int m){return fx.prepare(r,{m==1,.4f,1});}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_PLATE
#include "parable_plate.h"
namespace resource_bench {struct Plate:Holder<parable::dsp::Plate>{bool prepare(float r,int m){return fx.prepare(r,{m==2?.95f:.76f,5600,.34f,m==1});}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_ROOMS
#include "parable_coupled_rooms.h"
namespace resource_bench {struct Rooms:Holder<parable::soundforge::CoupledRooms>{Storage storage;bool prepare(float r,int m){storage.resize(fx.requiredSamples(r));return fx.prepare(r,storage.data(),storage.size(),{m==2?1.f:.85f,.65f,m==1});}float process(float x){return fx.process(x).mono;}};}
#endif
#ifdef BENCH_USE_TILED
#include "parable_tiled_room.h"
namespace resource_bench {struct Tiled:Holder<parable::soundforge::TiledRoom>{Storage storage;bool prepare(float r,int m){storage.resize(fx.requiredSamples(r));return fx.prepare(r,storage.data(),storage.size(),{m==2?1.2f:.62f,.35f,m==1});}float process(float x){return fx.process(x).mono;}};}
#endif
#ifdef BENCH_USE_HALL
#include "parable_hall.h"
namespace resource_bench {struct Hall:Holder<parable::dsp::Hall>{float input[64]{},left[64]{},right[64]{};unsigned pos=0;bool prepare(float r,int m){pos=0;std::fill_n(input,64,0);std::fill_n(left,64,0);std::fill_n(right,64,0);return fx.prepare(r,{m==2?1.f:.5f,.35f,m==1?.98f:0});}float process(float x){float y=.5f*(left[pos]+right[pos]);input[pos++]=x;if(pos==64){fx.process(input,left,right);pos=0;}return y;}};}
#endif
#ifdef BENCH_USE_CLOUD
#include "parable_chamber.h"
namespace resource_bench {struct Cloud:Holder<parable::soundforge::Chamber>{float input[64]{},output[64]{};unsigned pos=0;bool prepare(float r,int m){pos=0;std::fill_n(input,64,0);std::fill_n(output,64,0);return fx.prepare(r,m==1?24.f:m==2?64.f:.02f,.65f);}float process(float x){float y=output[pos];input[pos++]=x;if(pos==64){fx.process(input,output);pos=0;}return y;}};}
#endif
#ifdef BENCH_USE_DRIVE
#include "parable_drive.h"
namespace resource_bench {struct Drive:Holder<parable::dsp::Drive>{bool prepare(float r,int m){return fx.configure(r,{m==2?1.f:.5f,.5f,m==1?.9f:.25f,.7f});}float process(float x){return fx.process(x);}};}
#endif
#ifdef BENCH_USE_UTILITIES
#include "parable_core.h"
namespace resource_bench {
struct Utility {parable::Engine fx{48000};int kind=0;bool prepare(float r,int m){(void)m;if(r!=48000)return false;parable::Patch p; p.count=1;p.stages[0].type=kind==0?parable::Type::Bypass:kind==1?parable::Type::Gain:parable::Type::OnePoleLowpass;p.stages[0].parameter=kind==2?6000.f:1.5f;return fx.load(p)==parable::Status::Ok;}float process(float x){return fx.process(x);}Counters counters()const{return {};}};
}
#endif
namespace resource_bench {
inline Counters add(Counters a,Counters b){return {a.failures+b.failures,a.evaluations+b.evaluations,a.solves+b.solves,std::max(a.max_iterations,b.max_iterations),a.containment+b.containment};}
template<class Drive,class Reverb,class Delay,class Mod> struct Chain {
 Drive drive;Reverb reverb;Delay delay;Mod mod;float feedback=0;int route=0;
 bool prepare(float r,int m){return drive.prepare(r,m)&&reverb.prepare(r,m)&&delay.prepare(r,m)&&mod.prepare(r,m);}
 float process(float x){if(route==0)return reverb.process(delay.process(mod.process(drive.process(x))));
 if(route==1)return .3f*(std::isfinite(x)?std::clamp(x,-1.f,1.f):0.f)+.25f*drive.process(x)+.25f*reverb.process(x)+.1f*delay.process(x)+.1f*mod.process(x);
 // Explicit benchmark-only bounded feedback, state update AFTER the graph.
 float y=reverb.process(delay.process(mod.process(drive.process(x+.2f*feedback))));feedback=std::clamp(y,-1.f,1.f);return y;}
 Counters counters()const{return add(add(drive.counters(),reverb.counters()),add(delay.counters(),mod.counters()));}
};
}
