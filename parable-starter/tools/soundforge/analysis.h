#pragma once
// Host-only full-wave analysis. No model, audio callback or perceptual loudness API.
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace soundforge {
inline void require(bool condition, const char* message) {
    if (!condition) throw std::invalid_argument(message);
}

struct Stats { double rms = 0, peak = 0, mean = 0; };

inline Stats stats(const std::vector<float>& values) {
    require(!values.empty(), "Cannot measure an empty signal");
    Stats s;
    for (float x : values) {
        require(std::isfinite(x), "Nonfinite sample");
        s.rms += double(x)*x;
        s.peak = std::max(s.peak, std::abs(double(x)));
        s.mean += x;
    }
    s.rms = std::sqrt(s.rms/values.size());
    s.mean /= values.size();
    return s;
}

// Amplitude dB; a declared 1e-15 floor preserves LabRAT's -300 dB silence report.
inline double db(double amplitude) {
    require(std::isfinite(amplitude) && amplitude >= 0, "Invalid dB amplitude");
    return 20*std::log10(std::max(amplitude, 1e-15));
}

// Select one shared RMS target subject to each signal's crest factor. Does not
// change samples or conceal raw levels. The caller records and applies gains.
template <typename Signals>
double rmsTarget(const Signals& signals, double requestedRms, double peakLimit) {
    require(!signals.empty(), "No signals to match");
    require(std::isfinite(requestedRms) && requestedRms >= 0, "Invalid RMS target");
    require(std::isfinite(peakLimit) && peakLimit > 0 && peakLimit <= 1, "Invalid peak limit");
    double target = requestedRms;
    for (const auto& signal : signals) {
        const auto m = stats(signal);
        if (m.peak > 0) target = std::min(target, peakLimit*m.rms/m.peak);
    }
    return target;
}

inline double rmsGain(const Stats& raw, double target) {
    require(std::isfinite(raw.rms) && raw.rms >= 0 && std::isfinite(raw.peak)
            && raw.peak >= 0 && std::isfinite(raw.mean), "Invalid raw statistics");
    require(std::isfinite(target) && target >= 0, "Invalid RMS target");
    const double gain = raw.rms > 0 ? target/raw.rms : 1;
    require(std::isfinite(gain), "Nonfinite matching gain");
    return gain;
}
} // namespace soundforge
