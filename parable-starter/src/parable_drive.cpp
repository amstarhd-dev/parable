#include "parable_drive.h"
#include <algorithm>
#include <cmath>
namespace parable::dsp {
namespace {
constexpr double pi = 3.14159265358979323846;
float tidy(float x) noexcept { return std::fabs(x) < 1e-20f ? 0.f : x; }
float coefficient(float hz, float rate) noexcept {
    return static_cast<float>(1. - std::exp(-2. * pi * hz / rate));
}
}
Drive::Drive() noexcept { configure(48000.f); }
bool Drive::configure(float sampleRate, DriveParameters p, DriveRate mode) noexcept {
    if (!std::isfinite(sampleRate) || sampleRate < 32000 || sampleRate > 96000
        || !std::isfinite(p.drive) || !std::isfinite(p.tone)
        || !std::isfinite(p.bias) || !std::isfinite(p.level)
        || (mode != DriveRate::Oversampled4x && mode != DriveRate::Reference1x)) return false;
    p.drive = std::clamp(p.drive, 0.f, 1.f);
    p.tone = std::clamp(p.tone, 0.f, 1.f);
    p.bias = std::clamp(p.bias, -1.f, 1.f);
    p.level = std::clamp(p.level, 0.f, 1.f);
    parameters_ = p; rate_ = sampleRate; mode_ = mode;
    gain_ = 1.f + 39.f * p.drive * p.drive;
    highpassCoefficient_ = coefficient(80, rate_);
    preCoefficient_ = coefficient(6000, rate_);
    toneCoefficient_ = coefficient(1200 + 6800 * p.tone, rate_);
    dcCoefficient_ = coefficient(20, rate_);
    const double positive = .18 * (1. + .35 * p.bias);
    const double negative = .18 * (1. - .35 * p.bias);
    for (std::size_t i = 0; i < kCurveSize; ++i) {
        const double u = -16. + static_cast<double>(i) / 32.;
        // Monotonic resistor + antiparallel exponential diode equation.
        // Root is between zero and u. Fixed work, entirely off audio.
        double lo = std::min(0., u), hi = std::max(0., u);
        for (int iteration = 0; iteration < 48; ++iteration) {
            const double v = .5 * (lo + hi);
            const double f = v + .025 * (std::exp(v / positive) - std::exp(-v / negative));
            if (f < u) lo = v; else hi = v;
        }
        curve_[i] = static_cast<float>(.5 * (lo + hi));
    }
    double sum = 0;
    for (std::size_t i = 0; i < kTaps; ++i) {
        const double n = static_cast<double>(i) - (kTaps - 1) / 2.;
        const double sinc = n == 0 ? .2 : std::sin(2 * pi * .1 * n) / (pi * n);
        const double window = .42 - .5 * std::cos(2 * pi * i / (kTaps - 1))
                                    + .08 * std::cos(4 * pi * i / (kTaps - 1));
        fir_[i] = static_cast<float>(sinc * window); sum += fir_[i];
    }
    for (auto& h : fir_) h = static_cast<float>(h / sum);
    reset(); return true;
}
void Drive::reset() noexcept {
    upHistory_.fill(0); downHistory_.fill(0); upIndex_ = downIndex_ = 0;
    inputDc_ = pre_ = tone1_ = tone2_ = outputDc_ = 0;
}
float Drive::lowpass(float x, float& state, float a) noexcept {
    state = tidy(state + a * (x - state)); return state;
}
float Drive::curve(float x) const noexcept {
    const float position = std::fabs(std::clamp(x, -16.f, 16.f)) * 32.f;
    const auto i = static_cast<std::size_t>(position);
    constexpr auto center = (kCurveSize - 1) / 2;
    if (i >= center) return x < 0 ? curve_.front() : curve_.back();
    const auto index = x < 0 ? center - i : center + i;
    const auto next = x < 0 ? index - 1 : index + 1;
    return curve_[index] + (position - static_cast<float>(i)) * (curve_[next] - curve_[index]);
}
float Drive::process(float input) noexcept {
    // Discard nonfinite input without advancing history, matching native policy.
    if (!std::isfinite(input)) return 0;
    const float x = tidy(std::clamp(input, -4.f, 4.f));
    const float high = x - lowpass(x, inputDc_, highpassCoefficient_);
    const float pre = lowpass(high, pre_, preCoefficient_);
    float nonlinear = 0;
    if (mode_ == DriveRate::Reference1x) nonlinear = curve(pre * gain_);
    else {
        upIndex_ = upIndex_ == 0 ? upHistory_.size() - 1 : upIndex_ - 1;
        upHistory_[upIndex_] = pre;
        for (std::size_t phase = 0; phase < 4; ++phase) {
            float interpolated = 0;
            std::size_t history = upIndex_;
            for (std::size_t tap = phase; tap < kTaps; tap += 4) {
                interpolated += fir_[tap] * upHistory_[history];
                if (++history == upHistory_.size()) history = 0;
            }
            downIndex_ = downIndex_ == 0 ? kTaps - 1 : downIndex_ - 1;
            downHistory_[downIndex_] = tidy(curve(interpolated * (4.f * gain_)));
            if (phase == 0) {
                std::size_t index = downIndex_;
                for (std::size_t tap = 0; tap < kTaps; ++tap) {
                    nonlinear += fir_[tap] * downHistory_[index];
                    if (++index == kTaps) index = 0;
                }
            }
        }
    }
    const float tone = lowpass(lowpass(nonlinear, tone1_, toneCoefficient_), tone2_, toneCoefficient_);
    const float out = tidy((tone - lowpass(tone, outputDc_, dcCoefficient_)) * parameters_.level);
    if (!std::isfinite(out)) { reset(); return 0; } // Defensive internal-state recovery.
    return std::clamp(out, -1.f, 1.f);
}
void Drive::processBlock(const float* input, float* output, std::size_t count) noexcept {
    for (std::size_t i = 0; i < count; ++i) output[i] = process(input[i]);
}
} // namespace parable::dsp
