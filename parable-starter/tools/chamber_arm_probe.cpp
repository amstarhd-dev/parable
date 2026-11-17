#include <algorithm>
#include <array>
#include <memory>
#if ENGINE_CLOUD
#include "cloudseed/DSP/ReverbController.h"
using Core=Cloudseed::ReverbController;
extern "C" void chamber_process(Core* e,float**input,float**output,unsigned frames){e->Process(input[0],input[1],output[0],output[1],frames);}
#else
#include "basics/include/signalsmith-basics/reverb.h"
using Core=signalsmith::basics::ReverbFloat;
extern "C" void chamber_process(Core* e,float**input,float**output,unsigned frames){e->process(input,output,frames);}
extern "C" const unsigned atomic_double_lockfree=std::atomic<double>::is_always_lock_free;
#endif
extern "C" const unsigned chamber_size=sizeof(Core);
