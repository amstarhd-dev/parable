#pragma once
// SF-007 standalone mono rotary experiment; not a canonical Engine processor.
#include <array>
#include <cstdint>

namespace parable::dsp {
struct RotaryConfig { bool wornHornBelt=false; float mix=.65f; };
struct RotorMotion {
    double motorOmega=0, omega=0, motorAngle=0, angle=0, beltPhase=0;
    double driveNm=0, loadNm=0, beltNm=0, staticNm=0, requiredNm=0;
    double captureLossJ=0;
    uint32_t releases=0,captures=0;
    bool slipping=false,released=false,captured=false;
};
class Rotary {
public:
    unsigned interval=1,countdown=0;
    double hNow=1,bNow=1,hStep=0,bStep=0;
public:
    static constexpr uint32_t kDelayCells=384,kSineSegments=256;
    Rotary() noexcept=default;
    Rotary(const Rotary&)=delete;
    Rotary& operator=(const Rotary&)=delete;
    // Stopped control only: validate first; prepare coefficients/table and clear.
    bool prepare(float rate,RotaryConfig={}) noexcept;
    void reset() noexcept; // stopped only, resets mechanics/audio to prepared start.
    void stop() noexcept {running_=false;} // control, after audio quiescence.
    // Normalized mono input, speed command 0=slow / 1=fast; finite/clamped.
    // No allocation, locking, IO, coefficient preparation or variable work.
    float process(float input,float speedCommand) noexcept;
    const RotorMotion& horn() const noexcept {return horn_;} // same-thread only.
    const RotorMotion& drum() const noexcept {return drum_;}
    double hornDelaySamples() const noexcept {return delayH_;}
    double drumDelaySamples() const noexcept {return delayB_;}
    uint32_t containmentEvents() const noexcept {return containment_;}
    float hornContribution() const noexcept {return hornWet_;}
    float drumContribution() const noexcept {return drumWet_;}
private:
    double cosine(double angle) const noexcept;
    void advance(RotorMotion&,bool horn,double command) noexcept;
    RotaryConfig config_{};
    float rate_=0,crossover_=0,hornDark_=0,hornBright_=0,low_=0,color_=0,hornWet_=0,drumWet_=0;
    double dt_=0,delayH_=0,delayB_=0;
    uint32_t write_=0,containment_=0;
    bool running_=false;
    RotorMotion horn_{},drum_{};
    std::array<float,kDelayCells> lowDelay_{},highDelay_{};
    std::array<float,kSineSegments+1> sine_{};
};
static_assert(sizeof(Rotary)<=5120,"Gate0 complete rotary state ceiling");
} // namespace parable::dsp
