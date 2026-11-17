#pragma once
#include <cstddef>
#include <memory>
namespace parable::soundforge {
// Experimental HOST specimen. One owner, stopped prepare/reset/destruction.
// Complete borrowed stereo core is folded mono; no Engine/patch admission.
class Chamber {
public:
    static constexpr unsigned blockSize=64;
    static constexpr float healthyGain=.02f, faultGain=24.f;
    Chamber();~Chamber();
    Chamber(const Chamber&)=delete;Chamber& operator=(const Chamber&)=delete;
    bool prepare(float rate,float returnGain=healthyGain,float wet=.65f);
    // Input/output64 valid samples. Invalid input block produces zero and keeps
    // history; valid zero input continues the causal room/feedback response.
    bool process(const float* input,float* output) noexcept;
    // Same-owner gain request,20ms smoothing. Other preparation stays fixed.
    bool setReturnGain(float gain) noexcept;
    // Common reduced class-B amplifier: referred threshold7mV after fixedx100
    // mic preamp/Vbe.7V; symmetric finite output rails. Continuous, no RMS gate.
    static float returnAmplifier(float microphone,float gain) noexcept;
    const float* microphone()const noexcept{return mic_;}
    const float* speaker()const noexcept{return speaker_;}
    const float* returnSignal()const noexcept{return applied_;}
    static std::size_t coreBytes() noexcept;
    unsigned outputClamps()const noexcept{return outputClamps_;}
private:
    struct Core;std::unique_ptr<Core> core_;
    float memory_[64]{},mic_[64]{},speaker_[64]{},applied_[64]{};
    float gain_=0,targetGain_=0,gainPole_=0,wet_=0,hp_=0,lp_=0,prev_=0,hpPole_=0,lpPole_=0;
    unsigned outputClamps_=0;
};
}
