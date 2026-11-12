#pragma once
// SF-008 private modal contact; small N is used only for independent fixtures.
#include "parable_spring.h"
#include <cstddef>
#include <cmath>
namespace parable::dsp::spring_detail {
struct Contact {
    double before=0,after=0,velocityBefore=0,velocityAfter=0;
    double projectionLoss=0,restitutionLoss=0,impulse=0,highGain=0;
    bool projected=false,impacted=false;
};
template<std::size_t N>
Contact contact(std::array<SpringMode,N>& m,double gap,double restitution,double dd,double cc) noexcept {
    Contact out;
    for(const auto& mode:m){out.before+=mode.displacement*mode.x;out.velocityBefore+=mode.contact*mode.v;}
    out.after=out.before;out.velocityAfter=out.velocityBefore;
    if(out.before<=gap)return out; // ONE approached support, not two moved walls.
    out.projected=true;const double correction=(out.before-gap)/dd;
    out.projectionLoss=(out.before*out.before-gap*gap)/(2*dd);
    for(auto& mode:m)mode.x-=mode.displacement*correction;
    if(out.velocityBefore>0) {
        out.impacted=true;out.impulse=-(1+restitution)*out.velocityBefore/cc;
        out.restitutionLoss=(1-restitution*restitution)*out.velocityBefore*out.velocityBefore/(2*cc);
        for(auto& mode:m) {
            const double old=mode.v;mode.v+=out.impulse*mode.contact;
            if(mode.omega>12566.370614359172)out.highGain+=.5*(mode.v*mode.v-old*old);
        }
    }
    out.after=out.velocityAfter=0;
    for(const auto& mode:m){out.after+=mode.displacement*mode.x;out.velocityAfter+=mode.contact*mode.v;}
    return out;
}
} // namespace parable::dsp::spring_detail
