#pragma once
// SF-017 reduced Skywave-family laboratory; no Engine or patch registration.
#include <array>
#include <cstddef>
#include <cstdint>
namespace parable::dsp {
struct ScorpioControls {float gain=.5f,lowCut=.35f,level=.8f;bool resistorDrift=false;};
struct ScorpioProbe {float preampVolts=0,flux=0,magnetizingAmps=0,coreVolts=0,capacitorVolts=0,outputVolts=0;};
class Scorpio {
public:
    static constexpr unsigned kKnots=513,kFactor=4,kTaps=97;
    bool prepare(float rate,ScorpioControls c={},unsigned factor=4,bool saturating=true,bool adaa=true) noexcept;
    void reset() noexcept;
    void stop() noexcept {running_=false;}
    float process(float modelVolts) noexcept; // jackVolts/8; input containment +/-0.5V
    float curve(float x) const noexcept;
    const std::array<double,5>& bias() const noexcept {return bias_;} // B1,C1/B2,E1,E2,C2
    const ScorpioProbe& probe() const noexcept {return probe_;}
    float sourceOhms() const noexcept {return sourceOhms_;}
    uint32_t invalidInputs() const noexcept {return invalid_;}
    uint32_t inputClamps() const noexcept {return clamps_;}
    uint32_t failures() const noexcept {return failures_;}
    uint32_t tableClamps() const noexcept {return tableClamps_;}
private:
    struct Value {float y=0,F=0;};
    Value value(float) const noexcept;
    float sample(float) noexcept;
    float coefficient(unsigned i) const noexcept {return fir_[i<=48?i:96-i];}
    std::array<float,kKnots> x_{},y_{},slope_{},area_{};
    std::array<float,49> fir_{};
    std::array<float,25> up_{};
    std::array<float,97> down_{};
    std::array<std::array<float,5>,5> A_{};
    std::array<float,5> B_{},C_{},state_{};
    std::array<double,5> bias_{};
    ScorpioControls controls_{};ScorpioProbe probe_{};
    float D_=0,inverseGain_=1,sourceOhms_=0,dt_=0,phi_=0,cap_=0,previous_=0,previousArea_=0;
    float rSeries_=0,rPrimary_=0,rParallel_=0,rLoad_=0,sourceScale_=0,levelScale_=0;
    unsigned factor_=4,upIndex_=0,downIndex_=0;
    uint32_t invalid_=0,clamps_=0,failures_=0,tableClamps_=0;
    bool prepared_=false,running_=false,saturating_=true,adaa_=true;
};
static_assert(sizeof(Scorpio)<=12288,"Standalone SF017 state ceiling, not an Engine admission");
}
