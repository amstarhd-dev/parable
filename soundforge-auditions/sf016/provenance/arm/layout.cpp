#include "parable_blender.h"
#include "parable_activation.h"
extern "C" const uint32_t layout[]={sizeof(parable::dsp::Blender),sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};
