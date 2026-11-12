#pragma once
// SF-008 standalone passive modal spring surrogate; not a canonical processor.
#include <array>
#include <cstdint>
namespace parable::dsp {
struct SpringConfig {bool displacedSupport=false;float mix=.4f,wetReturn=1;};
struct SpringMode {
    double x=0,v=0,omega=0,decay=0,cosine=0,sine=0;
    double force=0,contact=0,displacement=0,pickup=0;
};
struct SpringProbe {
    double energyStartJ=0,energyEndJ=0,dampingLossJ=0,inputWorkJ=0;
    double projectionLossJ=0,restitutionLossJ=0,tailDiscardedJ=0,balanceErrorJ=0;
    double displacementBeforeM=0,displacementAfterM=0,penetrationM=0;
    double contactVelocityBefore=0,contactVelocityAfter=0,impulseNs=0;
    double impactHighGainJ=0,maximumImpactHighGainJ=0,highEnergyJ=0;
    double appliedForceN=0;
    uint32_t impacts=0,projections=0,inputLimits=0;
};
class Spring {
public:
    static constexpr uint32_t kModes=192,kSubsteps=2,kTaps=193;
    static constexpr double kEnergyLimitJ=2e-6,kRestitution=.55;
    static constexpr double kHealthyGapM=.005,kFaultGapM=.000005;
    bool prepare(float rate,SpringConfig={}) noexcept; // stopped control only.
    void reset() noexcept; // stopped; coefficients retained, zero audio/mechanics.
    void stop() noexcept {running_=false;} // control after audio quiesces.
    float process(float input) noexcept;
    const SpringProbe& probe() const noexcept {return probe_;} // same thread only.
    uint32_t impacts() const noexcept {return impacts_;}
    uint32_t inputLimits() const noexcept {return limits_;}
    uint32_t numericalFaults() const noexcept {return faults_;}
    uint32_t tailClears() const noexcept {return clears_;}
    double displacementBoundM() const noexcept {return displacementBound_;}
    double secondaryPickup() const noexcept {return secondPickup_;}
    double energyJ() const noexcept {return energy_;}
private:
    void substep(double forceN) noexcept;
    std::array<SpringMode,kModes> modes_{};
    std::array<double,kTaps> fir_{},down_{};
    SpringConfig config_{};
    SpringProbe probe_{};
    double step_=0,gap_=0,forceNorm2_=0,contactNorm2_=0,displacementNorm2_=0;
    double pickupNorm2_=0,returnScale_=0,energy_=0,secondPickup_=0,displacementBound_=0;
    uint32_t write_=0,silent_=0,tailFrames_=0,impacts_=0,limits_=0,faults_=0,clears_=0;
    bool running_=false,active_=false;
};
static_assert(sizeof(Spring)<=24576,"Gate0 complete spring state ceiling");
} // namespace parable::dsp
