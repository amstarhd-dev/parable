#pragma once
// SF012 experimental moving-medium record/playback; no Engine registration.
#include <array>
#include <cstddef>
#include <cstdint>
#if defined(__FAST_MATH__) || (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "Tape saturation requires IEEE finite checks"
#endif
namespace parable::dsp {
struct TapeSaturationProbe {
    double recordedVolts=0,remanence=0,flux_nWb_per_m=0;
    double headRemanence=0,coilVolts=0,outputVolts=0;
};
struct TapeParcelPoint {
    double relativeSeconds=0,field_A_per_m=0,magnetization=0;
    std::array<double,8> boundary_A_per_m{};
};
struct TapeParcelTrace {std::array<TapeParcelPoint,48> points{};std::size_t count=0;};
class TapeSaturation {
public:
    static constexpr std::size_t kRows=8,kPhases=8,kEvents=48,kHistory=256;
    static constexpr std::size_t kFactor=4,kTaps=193,kHeadTaps=257,kCDF=129;
    // One stopped owner; failed prepare preserves all old state. No live editing.
    bool prepare(float sampleRate,bool weakBias=false) noexcept;
    void reset() noexcept;
    void stop() noexcept {running_=false;}
    // Input model volts, containment at+/-2V (counted; not a physical limiter).
    float process(float inputVolts) noexcept; // software outputVolts/8
    // Host science only: bypass electronics/resamplers, advance ONE material
    // tick at4*prepared sampleRate. Input recorded driver volts; result M/Ms.
    // Do not mix this entry with process() without reset. Same owner/thread.
    float materialReference(float recordedVolts) noexcept;
    // Analysis only, outside timed/audio processing; actual latest history and
    // same parcel implementation. Phase is averaging quadrature, not a clock.
    bool traceParcel(std::size_t phase,TapeParcelTrace&) const noexcept;
    const TapeSaturationProbe& probe() const noexcept {return probe_;}
    uint32_t failures() const noexcept {return failures_;}
    uint32_t invalidInputs() const noexcept {return invalid_;}
    uint32_t inputClamps() const noexcept {return inputClamps_;}
    uint32_t outputClamps() const noexcept {return outputClamps_;}
    uint32_t parcels() const noexcept {return parcels_;}
    double biasAmps() const noexcept {return weak_?.006:.012;}
    double internalRate() const noexcept {return rate_*kFactor;}
    double saturationFluxPerWidth() const noexcept {return fluxUnit_;} // Wb/m
    const std::array<float,kHeadTaps>& playbackKernel() const noexcept {return head_;} // host analysis
private:
    struct Event {uint32_t lag=0;float fraction=0,audio=0,bias=0;};
    double parcel(std::size_t,TapeParcelTrace*) const noexcept;
    double material(float) noexcept;
    float coefficient(std::size_t n) const noexcept {return fir_[n<fir_.size()?n:kTaps-1-n];}
    std::array<std::array<Event,kEvents>,kPhases> events_{};
    std::array<uint32_t,kPhases> counts_{};
    std::array<float,kCDF> cdf_{},slope_{};
    std::array<double,kRows> weights_{};
    std::array<float,kHistory> history_{};
    std::array<float,(kTaps+1)/2> fir_{};
    std::array<float,kTaps> down_{};
    std::array<float,(kTaps+kFactor-1)/kFactor> up_{};
    std::array<float,kHeadTaps> head_{},headHistory_{};
    double rate_=0,b0_=0,b1_=0,a1_=0,recordX_=0,recordY_=0,hpG_=0,hpState_=0;
    double fluxUnit_=0,previousHead_=0;
    uint32_t historyIndex_=0,upIndex_=0,downIndex_=0,headIndex_=0;
    uint32_t failures_=0,invalid_=0,inputClamps_=0,outputClamps_=0,parcels_=0;
    bool prepared_=false,running_=false,weak_=false;
    TapeSaturationProbe probe_{};
};
static_assert(sizeof(TapeSaturation)<=16384,"SF012 standalone state budget");
} // namespace parable::dsp
