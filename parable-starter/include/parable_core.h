#pragma once
// Parable Core — native C++ architecture proof, v0.1.
// No heap allocation, file/network calls, or exceptions in audio callback.
// This is intentionally NOT a pedal-grade distortion library.
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace parable {
constexpr uint32_t kSchema = 2;
constexpr size_t kMaxStages = 8;
constexpr size_t kPresets = 4;

enum class Type : uint8_t { Bypass = 0, Gain = 1, OnePoleLowpass = 2 };

struct Stage {
    Type type = Type::Bypass;
    float parameter = 1.0f;   // Gain: linear amplitude 0..2; lowpass: cutoff Hz 20..20000
    bool enabled = true;
};

struct Patch {
    uint32_t schema = kSchema;
    uint8_t count = 0;
    std::array<Stage, kMaxStages> stages{}; // ordered serial path; later graph form is deliberately deferred
    float outputTrim = 1.0f; // 0..1; safety margin / user VOL handled separately
};

enum class Status : uint8_t {
    Ok, SchemaMismatch, TooManyStages, InvalidParameter, InvalidType,
    InvalidSlot, CorruptImage, ImageVersionMismatch
};

inline bool finite(float v) noexcept { return std::isfinite(v); }

inline Status validate(const Patch& p, float sampleRate) noexcept {
    if (p.schema != kSchema) return Status::SchemaMismatch;
    if (p.count > kMaxStages) return Status::TooManyStages;
    if (!finite(p.outputTrim) || p.outputTrim < 0.f || p.outputTrim > 1.f)
        return Status::InvalidParameter;
    if (!finite(sampleRate) || sampleRate < 8000.f || sampleRate > 192000.f)
        return Status::InvalidParameter;
    for (size_t i = 0; i < p.count; ++i) {
        const Stage& s = p.stages[i];
        if (!finite(s.parameter)) return Status::InvalidParameter;
        switch (s.type) {
            case Type::Bypass: break;
            case Type::Gain:
                if (s.parameter < 0.f || s.parameter > 2.f) return Status::InvalidParameter;
                break;
            case Type::OnePoleLowpass:
                if (s.parameter < 20.f || s.parameter >= 0.45f * sampleRate)
                    return Status::InvalidParameter;
                break;
            default: return Status::InvalidType;
        }
    }
    return Status::Ok;
}

inline Patch makeBypassPatch() noexcept { return {}; }

// Owns validated snapshots. No pointers to mutable UI objects or chart state.
class Bank {
public:
    explicit Bank(float sampleRate) noexcept : rate_(sampleRate) {
        for (auto& p : patches_) p = makeBypassPatch();
    }
    Status set(size_t slot, const Patch& source) noexcept {
        if (slot >= kPresets) return Status::InvalidSlot;
        auto ok = validate(source, rate_);
        if (ok != Status::Ok) return ok;
        patches_[slot] = source;  // commit only after validation
        return Status::Ok;
    }
    const Patch& get(size_t slot) const noexcept { return patches_[slot]; }
    Status select(size_t slot) noexcept {
        if (slot >= kPresets) return Status::InvalidSlot;
        active_ = static_cast<uint8_t>(slot);
        return Status::Ok;
    }
    size_t active() const noexcept { return active_; }
    float sampleRate() const noexcept { return rate_; }
private:
    float rate_;
    size_t active_ = 0;
    std::array<Patch, kPresets> patches_{};
};

// Proof-of-plumbing DSP. One-pole LPF is a utility processor, not a modeled pedal.
// load() is for control thread ONLY; call when audio callback is stopped in this prototype.
class Engine {
public:
    explicit Engine(float sr) noexcept : sampleRate_(sr) { load(makeBypassPatch()); }
    Status load(const Patch& patch) noexcept {
        Status status = validate(patch, sampleRate_);
        if (status != Status::Ok) return status;
        patch_ = patch;
        states_.fill(0.f);
        for (size_t i=0;i<patch_.count;++i) {
            auto& s = patch_.stages[i];
            if (s.type == Type::OnePoleLowpass) {
                // Simple stable one-pole low-pass coefficient at selected sample rate.
                a_[i] = 1.f - std::exp(-6.283185307179586f * s.parameter / sampleRate_);
            } else a_[i] = 0.f;
        }
        return Status::Ok;
    }
    void setVolume(float volume) noexcept { volume_ = finite(volume) ? clip(volume, 0.f, 1.f) : 0.f; }
    float process(float input) noexcept {
        // Discard non-finite samples without advancing filter history.
        if (!finite(input)) return 0.f;
        float x = clip(input, -kInternalHeadroom, kInternalHeadroom);
        for (size_t i=0;i<patch_.count;++i) {
            const auto& s = patch_.stages[i];
            if (!s.enabled) continue;
            switch (s.type) {
                case Type::Bypass: break;
                case Type::Gain:
                    x = clip(x * s.parameter, -kInternalHeadroom, kInternalHeadroom);
                    break;
                case Type::OnePoleLowpass:
                    states_[i] = clip(states_[i] + a_[i] * (x - states_[i]),
                                      -kInternalHeadroom, kInternalHeadroom);
                    x = states_[i];
                    break;
            }
        }
        // Output limiting is a hard safety clamp, NOT a mastered level-management scheme.
        return clip(x * patch_.outputTrim * volume_, -1.f, 1.f);
    }
    const Patch& loaded() const noexcept { return patch_; }
private:
    // Numerical safety rail: 2^8 permits all eight gain-2 stages for |input| <= 1.
    // Bounding input, stages and filter history keeps float intermediates finite.
    // This is emergency saturation, not an analog model or a complete limiter.
    static constexpr float kInternalHeadroom = 256.f;
    static float clip(float x,float lo,float hi) noexcept {return x<lo?lo:(x>hi?hi:x);}
    float sampleRate_;
    float volume_ = 1.f;
    Patch patch_{};
    std::array<float,kMaxStages> states_{};
    std::array<float,kMaxStages> a_{};
};
} // namespace parable
