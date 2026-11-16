#pragma once
// SF-019: standalone Jupiter/Taurus experimental plate-character reverberator.
// Dattorro 1997 topology independently adapted from MIT-licensed C/C++ examples.
// NOT a measured EMT 140 or a finite-element steel plate simulation.
#include <array>
#include <cstdint>
#include <vector>
namespace parable::dsp {
struct PlateControls {
    float decay = 0.76f;   // 0 ... 0.95
    float dampingHz = 5600.f;
    float wet = 0.34f;
    bool damagedMount = false;
};
struct PlateProbe {
    float suspensionDisplacement = 0.f; // reduced, dimensionless mechanical coordinate
    float suspensionVelocity = 0.f;     // displacement units per second
    float contactImpulse = 0.f;         // postminuspre velocity, dimensionless per second
    float wetLeft = 0.f, wetRight = 0.f;
    std::uint64_t contacts = 0;
    float peakDisplacement = 0.f;
};
class Plate {
public:
    bool prepare(float sampleRate, PlateControls controls = {}) noexcept;
    void reset() noexcept;
    void stop() noexcept { running_ = false; }
    float process(float sample) noexcept;
    const PlateProbe& probe() const noexcept { return probe_; }
    std::size_t bufferBytes() const noexcept;
    bool running() const noexcept { return running_; }
    std::uint32_t invalidSamples() const noexcept { return invalid_; }
    std::uint32_t clippedInputs() const noexcept { return clipped_; }
    std::uint32_t nonfiniteOutputs() const noexcept { return nonfinite_; }
private:
    struct Delay {
        std::vector<float> data;
        unsigned pos = 0;
        float tap(float samples) const noexcept;
        void push(float value) noexcept;
        void reset() noexcept;
    };
    static constexpr unsigned count = 13;
    std::array<Delay,count> lines_{}; // pre + twelve Dattorro lines
    std::array<float,12> lengths_{};
    PlateControls controls_{};
    PlateProbe probe_{};
    float sampleRate_ = 0.f;
    float inputLP_ = 0.f, dampLP_L_ = 0.f, dampLP_R_ = 0.f;
    float inCoeff_ = 0.f, dampCoeff_ = 0.f;
    float oscX_ = 0.f, oscV_ = 0.f, collisionReturn_ = 0.f;
    float sinePhaseA_ = 0.f, sinePhaseB_ = 0.f;
    int contactSide_ = 0;
    std::array<float,257> sine_{};
    float predelay_ = 0.f;
    float modFreqA_ = 0.f, modFreqB_ = 0.f;
    bool running_ = false;
    std::uint32_t invalid_ = 0, clipped_ = 0, nonfinite_ = 0;
    float apf(unsigned i, float x, float gain, float d) noexcept;
    float decayTap(unsigned i, float d) const noexcept { return lines_[i+1].tap(d); }
};
} // namespace parable::dsp
