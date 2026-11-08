#pragma once
// Experimental PAR-006 kernel; ADR-002 is proposed, musical approval pending.
#include <array>
#include <cstddef>
#if defined(__FAST_MATH__) || (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "Parable drive requires IEEE-aware finite checks"
#endif
namespace parable::dsp {
struct DriveParameters {
    float drive = .3f; // [0,1], pre-gain 1 + 39*drive^2
    float tone = .5f;  // [0,1], two low-passes 1200 + 6800*tone Hz
    float bias = .25f; // [-1,1], relative exponential diode knees
    float level = .7f; // [0,1], output amplitude multiplier
};
enum class DriveRate { Oversampled4x, Reference1x }; // 1x for lab comparisons only.
class Drive {
public:
    Drive() noexcept;
    // Control side, stopped audio only. Reject nonfinite/rate/mode errors without
    // changing current state; clamp finite controls. Success resets all history.
    bool configure(float sampleRate, DriveParameters parameters = {},
                   DriveRate mode = DriveRate::Oversampled4x) noexcept;
    void reset() noexcept;
    float process(float input) noexcept;
    // Supports in-place buffers; caller supplies valid arrays of count elements.
    void processBlock(const float* input, float* output, std::size_t count) noexcept;
    DriveParameters parameters() const noexcept { return parameters_; }
    float sampleRate() const noexcept { return rate_; }
    static constexpr std::size_t kTaps = 193;
    static constexpr std::size_t kCurveSize = 1025;
    static constexpr std::size_t kFirDelaySamples = 48; // IIR delay is additional.
private:
    float curve(float x) const noexcept;
    static float lowpass(float x, float& history, float coefficient) noexcept;
    std::array<float, kCurveSize> curve_{};
    std::array<float, kTaps> fir_{};
    std::array<float, (kTaps + 3) / 4> upHistory_{};
    std::array<float, kTaps> downHistory_{};
    std::size_t upIndex_ = 0, downIndex_ = 0;
    float inputDc_ = 0, pre_ = 0, tone1_ = 0, tone2_ = 0, outputDc_ = 0;
    float highpassCoefficient_ = 0, preCoefficient_ = 0, toneCoefficient_ = 0, dcCoefficient_ = 0;
    float gain_ = 1, rate_ = 48000;
    DriveParameters parameters_{};
    DriveRate mode_ = DriveRate::Oversampled4x;
};
} // namespace parable::dsp
