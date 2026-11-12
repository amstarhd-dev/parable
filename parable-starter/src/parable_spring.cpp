// Positive stiffness/stretch and cosine mode placement adapted narrowly from
// Rings, Copyright 2015 Emilie Gillet (MIT), notice in third_party/spring.
// Explicit energy coordinates, split loss and unilateral impact are local work.
#include "parable_spring.h"
#include "spring_contact.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace parable::dsp {
namespace {
constexpr double pi=3.14159265358979323846,mass=.0003;
void increment(uint32_t& value) noexcept {if(value!=std::numeric_limits<uint32_t>::max())++value;}
double energy(const SpringMode& m) noexcept {return .5*(m.x*m.x+m.v*m.v);}
}
bool Spring::prepare(float rate,SpringConfig c) noexcept {
    if(!std::isfinite(rate)||rate<32000||rate>96000||!std::isfinite(c.mix)||c.mix<0||c.mix>1
       ||!std::isfinite(c.wetReturn)||c.wetReturn<0||c.wetReturn>1)return false;
    config_=c;step_=1/(double(rate)*kSubsteps);gap_=c.displacedSupport?kFaultGapM:kHealthyGapM;
    forceNorm2_=contactNorm2_=displacementNorm2_=pickupNorm2_=0;
    for(uint32_t branch=0;branch<2;++branch) {
        double stretch=1,stiffness=.04;
        for(uint32_t i=0;i<kModes/2;++i) {
            auto& m=modes_[branch*(kModes/2)+i];const double n=i+1,f=(branch?31:26)*n*stretch;
            m.omega=2*pi*f;m.decay=std::exp(-(3+8*(f/8000)*(f/8000))*step_);
            m.cosine=std::cos(m.omega*step_);m.sine=std::sin(m.omega*step_);
            m.force=.5*std::cos(n*pi*.03)/std::sqrt(mass);
            m.contact=branch==0?std::cos(n*pi*.37)/std::sqrt(mass):0;
            m.displacement=m.contact/m.omega;m.pickup=.5*std::cos(n*pi*(branch?.88:.91))/std::sqrt(mass);
            forceNorm2_+=m.force*m.force;contactNorm2_+=m.contact*m.contact;
            displacementNorm2_+=m.displacement*m.displacement;pickupNorm2_+=m.pickup*m.pickup;
            stretch+=stiffness;stiffness*=.98;
        }
    }
    // Existing owned FIR idiom; 2x output anti-alias, not a reverb/ghost delay.
    double sum=0;
    for(uint32_t i=0;i<kTaps;++i) {
        const double n=double(i)-(kTaps-1)/2.;
        fir_[i]=(n==0?.42:std::sin(2*pi*.21*n)/(pi*n))
            *(.42-.5*std::cos(2*pi*i/(kTaps-1))+.08*std::cos(4*pi*i/(kTaps-1)));
        sum+=fir_[i];
    }
    double l1=0;for(auto& value:fir_){value/=sum;l1+=std::abs(value);}
    returnScale_=.8/(l1*std::sqrt(2*kEnergyLimitJ*pickupNorm2_));
    displacementBound_=std::sqrt(2*kEnergyLimitJ*displacementNorm2_);
    tailFrames_=static_cast<uint32_t>(std::ceil(6*double(rate)));
    running_=true;reset();return true;
}
void Spring::reset() noexcept {
    for(auto& m:modes_)m.x=m.v=0;
    down_.fill(0);probe_={};energy_=secondPickup_=0;write_=silent_=impacts_=limits_=faults_=clears_=0;active_=false;
    running_=step_>0;
}
void Spring::substep(double requestedForce) noexcept {
    const double start=energy_;double damped=0,cross=0;
    for(auto& m:modes_) {m.x*=m.decay;m.v*=m.decay;damped+=energy(m);cross+=m.v*m.force;}
    probe_.dampingLossJ+=start-damped;
    double impulse=requestedForce*step_;
    double a=impulse*cross,b=.5*impulse*impulse*forceNorm2_;
    if(damped+a+b>kEnergyLimitJ) {
        const double room=std::max(0.,kEnergyLimitJ-damped),root=std::sqrt(a*a+4*b*room);
        double scale=a>=0?2*room/(a+root):(-a+root)/(2*b);
        scale=std::max(0.,std::min(1.,scale));scale=std::nextafter(scale,0.);
        impulse*=scale;++probe_.inputLimits;increment(limits_);
    }
    probe_.appliedForceN=impulse/step_;
    double kicked=0;
    for(auto& m:modes_) {
        m.v+=impulse*m.force;kicked+=energy(m);
        const double old=m.x;m.x=m.cosine*m.x+m.sine*m.v;m.v=m.cosine*m.v-m.sine*old;
    }
    probe_.inputWorkJ+=kicked-damped;
    const auto c=spring_detail::contact(modes_,gap_,kRestitution,displacementNorm2_,contactNorm2_);
    probe_.displacementBeforeM=std::max(probe_.displacementBeforeM,c.before);
    probe_.displacementAfterM=c.after;
    if(c.impacted||probe_.impacts==0) {
        probe_.contactVelocityBefore=c.velocityBefore;probe_.contactVelocityAfter=c.velocityAfter;
    }
    probe_.penetrationM=std::max(probe_.penetrationM,std::max(0.,c.before-gap_));
    probe_.projectionLossJ+=c.projectionLoss;probe_.restitutionLossJ+=c.restitutionLoss;probe_.impulseNs+=c.impulse;
    probe_.impactHighGainJ+=c.highGain;probe_.maximumImpactHighGainJ=std::max(probe_.maximumImpactHighGainJ,c.highGain);
    if(c.projected)++probe_.projections;
    if(c.impacted){++probe_.impacts;increment(impacts_);}
    double pickup=0;secondPickup_=energy_=probe_.highEnergyJ=0;
    for(uint32_t i=0;i<kModes;++i){const auto& m=modes_[i];energy_+=energy(m);pickup+=m.pickup*m.v;
        if(i>=kModes/2)secondPickup_+=m.pickup*m.v;
        if(m.omega>2*pi*2000)probe_.highEnergyJ+=energy(m);
    }
    if(!std::isfinite(energy_)||energy_>kEnergyLimitJ*(1+1e-10)) {
        // Containment is visible and must be zero in all audition/reference runs.
        for(auto& m:modes_)m.x=m.v=0;
        energy_=pickup=secondPickup_=0;increment(faults_);
    }
    write_=write_==0?kTaps-1:write_-1;down_[write_]=pickup*returnScale_;
}
float Spring::process(float input) noexcept {
    if(!running_)return 0;
    const double x=std::isfinite(input)?std::max(-1.,std::min(1.,double(input))):0;
    probe_={};probe_.energyStartJ=energy_;
    if(x!=0){active_=true;silent_=0;}
    else if(active_)++silent_;
    if(active_)for(uint32_t i=0;i<kSubsteps;++i)substep(.002*x);
    if(active_&&silent_>=tailFrames_) {
        probe_.tailDiscardedJ=energy_;for(auto& m:modes_)m.x=m.v=0;
        energy_=secondPickup_=0;down_.fill(0);active_=false;increment(clears_);
    }
    probe_.energyEndJ=energy_;
    probe_.balanceErrorJ=energy_-probe_.energyStartJ+probe_.dampingLossJ-probe_.inputWorkJ
        +probe_.projectionLossJ+probe_.restitutionLossJ+probe_.tailDiscardedJ;
    double wet=0;uint32_t index=write_;
    for(uint32_t i=0;i<kTaps;++i){wet+=fir_[i]*down_[index];if(++index==kTaps)index=0;}
    return float((1-config_.mix)*x+config_.mix*config_.wetReturn*wet);
}
} // namespace parable::dsp
