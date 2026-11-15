#pragma once
// SF014 standalone experimental loaded Lockhart cell. No Engine registration.
#include <array>
#include <cstddef>
#include <cstdint>
#if defined(__FAST_MATH__) || (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "Wavefolder requires IEEE finite checks"
#endif
namespace parable::dsp {
struct WavefolderControls {float drive=1;bool highFeedResistor=false;};
struct WavefolderProbe {double cellInputVolts=0,instantCellVolts=0,averagedCellVolts=0,filteredVolts=0,outputVolts=0;};
class Wavefolder {
public:
    static constexpr std::size_t kIntervals=512,kKnots=513,kMaxFactor=4,kMaxTaps=193;
    // One stopped owner. A rejected prepare preserves the old circuit/history.
    // Normal quality is4x+ADAA. Other factors1/2/4 and ADAA-off are diagnostics.
    bool prepare(float rate,WavefolderControls controls={},unsigned factor=4,bool antialias=true) noexcept;
    void reset() noexcept;
    void stop() noexcept {running_=false;}
    // Same owner, between samples; no smoothing/clickless control promise.
    bool setDrive(float drive) noexcept;
    float process(float inputModelVolts) noexcept; // outputVolts/8, input containment+/- .25V
    struct CellValue {double volts=0,primitive=0;};
    CellValue cellCurve(double inputModelVolts) const noexcept; // immutable host science; domain[-3,3]
    const WavefolderProbe& probe() const noexcept {return probe_;}
    const WavefolderControls& controls() const noexcept {return controls_;}
    double zeroInputVolts() const noexcept {return dc_;}
    double upperFeedOhms() const noexcept {return controls_.highFeedResistor?22000:15000;}
    unsigned factor() const noexcept {return factor_;}
    bool antialias() const noexcept {return antialias_;}
    uint32_t invalidInputs() const noexcept {return invalid_;}
    uint32_t inputClamps() const noexcept {return inputClamps_;}
    uint32_t tableClamps() const noexcept {return tableClamps_;}
    uint32_t outputClamps() const noexcept {return outputClamps_;}
    uint32_t failures() const noexcept {return failures_;}
private:
    double sample(double) noexcept;
    float coefficient(unsigned n) const noexcept {return fir_[n<=(taps_-1)/2?n:taps_-1-n];}
    std::array<float,kKnots> values_{},slopes_{};
    std::array<double,kKnots> integrals_{};
    std::array<float,97> fir_{};
    std::array<float,49> up_{};
    std::array<float,kMaxTaps> down_{};
    WavefolderControls controls_{};WavefolderProbe probe_{};
    double rate_=0,lastX_=0,lastF_=0,hpState_=0,hpG_=0,dc_=0;
    unsigned factor_=4,taps_=193,upIndex_=0,downIndex_=0;
    uint32_t invalid_=0,inputClamps_=0,tableClamps_=0,outputClamps_=0,failures_=0;
    bool antialias_=true,prepared_=false,running_=false;
};
static_assert(sizeof(Wavefolder)<=12288,"SF014 standalone state ceiling; not an Engine pool change");
} // namespace parable::dsp
