#pragma once
// Build-time laboratory variants only. No runtime mode/member/cache is added.
#ifndef PARABLE_GERMANIUM_EVALUATOR
#define PARABLE_GERMANIUM_EVALUATOR 0 // Remain libm until equivalence/performance gates select.
#endif
#if PARABLE_GERMANIUM_EVALUATOR == 0
#include "germanium_device.h"
namespace parable::dsp::germanium_detail {
inline Current deviceSelected(double ve,double vc,const GermaniumDevice& p) noexcept {return device(ve,vc,p);}
}
#else
#include "germanium_device_fast.h"
namespace parable::dsp::germanium_detail {
#if PARABLE_GERMANIUM_EVALUATOR == 1
using SelectedExponential=Hermite32;
#elif PARABLE_GERMANIUM_EVALUATOR == 2
using SelectedExponential=Hermite64;
#elif PARABLE_GERMANIUM_EVALUATOR == 3
using SelectedExponential=Polynomial8;
#elif PARABLE_GERMANIUM_EVALUATOR == 4
using SelectedExponential=Polynomial12;
#else
#error Unknown build-time germanium evaluator
#endif
inline Current deviceSelected(double ve,double vc,const GermaniumDevice& p) noexcept {return deviceApprox<SelectedExponential>(ve,vc,p);}
}
#endif
