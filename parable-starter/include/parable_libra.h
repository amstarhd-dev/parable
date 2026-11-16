#pragma once
// SF013 reduced complementary class-AB guitar instrument. Standalone only.
#include <array>
#include <cstdint>
namespace parable::dsp {
struct LibraControls {float drive=.5f,level=.8f;bool damagedBias=false;};
struct LibraProbe {float driverVolts=0,combinedVolts=0,upperAmps=0,lowerAmps=0,errorVolts=0,poleResidualVolts=0,jackVolts=0;bool railed=false,valid=false;};
class Libra {
public:
    // Stopped owner only;1x/2x are diagnostics. Nominal stage load1k.
    bool prepare(float rate,LibraControls c={},unsigned factor=4,bool feedback=true,float loadOhms=1000) noexcept;
    void reset() noexcept;void stop() noexcept {running_=false;}
    float process(float inputModelVolts) noexcept; // jackV/4; input domain+/-0.5V
    LibraProbe dc(float voltageAtGainInput=0) const noexcept;
    const LibraProbe& bias() const noexcept {return bias_;}
    const LibraProbe& probe() const noexcept {return probe_;}
    float gain() const noexcept {return gain_;}float feedbackRatio() const noexcept {return beta_;}
    float effectiveLoad() const noexcept {return load_;}
    uint32_t failures() const noexcept {return failures_;}uint32_t inputClamps() const noexcept {return clamps_;}
    uint32_t invalidInputs() const noexcept {return invalid_;}uint32_t railSamples() const noexcept {return rails_;}
private:
    struct Region {float m=0,c=0,dynamicInv=0,dynamicOffset=0,dcInv=0,dcOffset=0;unsigned mask=0;};
    LibraProbe select(float drive,bool dc) const noexcept;float sample(float) noexcept;
    float coefficient(unsigned n) const noexcept {return fir_[n<=(taps_-1)/2?n:taps_-1-n];}
    std::array<Region,4> regions_{};std::array<float,49> fir_{};std::array<float,25> up_{};std::array<float,97> down_{};
    LibraControls controls_{};LibraProbe bias_{},probe_{};
    float aN_=0,bN_=0,aP_=0,bP_=0,gN_=0,gP_=0,gain_=0,beta_=0,load_=0,k_=0,driver_=0,inputG_=0,inputZ_=0,outputG_=0,outputZ_=0;
    unsigned factor_=4,taps_=97,upIndex_=0,downIndex_=0;uint32_t failures_=0,clamps_=0,invalid_=0,rails_=0;
    bool prepared_=false,running_=false;
};
static_assert(sizeof(Libra)<=2048,"SF013 standalone state ceiling, not Engine admission");
}
