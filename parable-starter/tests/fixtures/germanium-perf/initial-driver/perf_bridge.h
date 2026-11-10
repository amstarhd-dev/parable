#pragma once
#include <cstddef>
#include <cstdint>
struct PerfProbe {double value[6];bool valid;};
struct PerfState {double nodes[8],bias[6];std::uint64_t calls,evals,failed,contained;unsigned maximum,iterations;};
struct PerfApi {
 const char* name;
 void* (*create)();void (*destroy)(void*);bool (*configure)(void*,float,float,float,int,const void*,std::size_t);
 void (*reset)(void*);void (*block)(void*,const float*,float*,std::size_t);PerfState (*state)(const void*);
 void (*device)(const void*,std::size_t,const double*,const double*,PerfProbe*,std::size_t);
};
extern "C" const PerfApi* perf_libm();
