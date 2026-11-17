#pragma once
#include "DSP/ReverbController.h"
#include "Programs.h"
#include <cmath>
namespace parable::chamber_detail {
// Existing core and parameter APIs; no delay/scattering/decay equation changes.
inline void compact(Cloudseed::ReverbController& e) {
    using namespace Cloudseed;
    initPrograms();
    for(int i=0;i<Parameter::COUNT;++i)e.SetParameter(i,ProgramDarkPlate[i]);
    e.SetParameter(Parameter::DryOut,0);
    e.SetParameter(Parameter::LateLineDecay,.25);
    e.SetParameter(Parameter::LateLineSize,.2);
    e.SetParameter(Parameter::HighCutEnabled,1);
    e.SetParameter(Parameter::HighCut,std::log2(1+15*(6500.-400)/19600)/4);
    e.SetParameter(Parameter::LowCut,std::log2(1+15*(120.-20)/980)/4);
    e.SetParameter(Parameter::TapEnabled,1);
    e.SetParameter(Parameter::TapCount,.035);
    e.SetParameter(Parameter::TapLength,.08);
    e.SetParameter(Parameter::EarlyOut,.5);
    e.ClearBuffers();
}
}
