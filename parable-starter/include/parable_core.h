#pragma once
// Parable Core — native C++ architecture proof, v0.1.
// No heap allocation, file/network calls, or exceptions in audio callback.
// This is intentionally NOT a pedal-grade distortion library.
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <new>
#include "parable_rat.h"
#include "parable_cmos.h"

namespace parable {
constexpr uint32_t kSchema = 2;
constexpr size_t kMaxStages = 8;
constexpr size_t kPresets = 4;

enum class Type : uint8_t { Bypass = 0, Gain = 1, OnePoleLowpass = 2, Rat = 3, GeminiCmos = 4 };
struct GeminiStageControls { float drive, volume; bool biasLeak; };

struct Stage {
    Type type = Type::Bypass;
    float parameter = 1.0f;   // Gain: linear amplitude 0..2; lowpass: cutoff Hz 20..20000
    bool enabled = true;
    // Same 12-byte parameter space. Explicit setters establish union lifetimes.
    union { dsp::RatControls rat{}; GeminiStageControls gemini; };
    Stage(Type t=Type::Bypass,float p=1.f,bool on=true,dsp::RatControls controls={}) noexcept
        :type(t),parameter(p),enabled(on),rat(controls) {
        if(t==Type::GeminiCmos)setGemini({.5f,.5f,false});
    }
    void setRat(dsp::RatControls controls) noexcept {::new(static_cast<void*>(&rat)) dsp::RatControls(controls);type=Type::Rat;}
    void setGemini(GeminiStageControls controls) noexcept {::new(static_cast<void*>(&gemini)) GeminiStageControls(controls);type=Type::GeminiCmos;}
};

struct Patch {
    uint32_t schema = kSchema;
    uint8_t count = 0;
    std::array<Stage, kMaxStages> stages{}; // ordered serial path; later graph form is deliberately deferred
    float outputTrim = 1.0f; // 0..1; safety margin / user VOL handled separately
};

enum class Status : uint8_t {
    Ok, SchemaMismatch, TooManyStages, InvalidParameter, InvalidType,
    InvalidSlot, CorruptImage, ImageVersionMismatch, TooManyRats, UnsupportedRatRate,
    TooManyNonlinear, UnsupportedCmosRate
};

inline bool finite(float v) noexcept { return std::isfinite(v); }

inline Status validate(const Patch& p, float sampleRate) noexcept {
    if (p.schema != kSchema) return Status::SchemaMismatch;
    if (p.count > kMaxStages) return Status::TooManyStages;
    if (!finite(p.outputTrim) || p.outputTrim < 0.f || p.outputTrim > 1.f)
        return Status::InvalidParameter;
    if (!finite(sampleRate) || sampleRate < 8000.f || sampleRate > 192000.f)
        return Status::InvalidParameter;
    size_t rats = 0,cmos = 0;
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
            case Type::Rat:
                if (++rats > 1) return Status::TooManyRats; // Disabled declarations also consume the slot.
                if(cmos) return Status::TooManyNonlinear;
                if (sampleRate < 32000.f || sampleRate > 96000.f) return Status::UnsupportedRatRate;
                for (float control : {s.rat.distortion, s.rat.filter, s.rat.volume})
                    if (!finite(control) || control < 0.f || control > 1.f) return Status::InvalidParameter;
                break;
            case Type::GeminiCmos:
                if(++cmos+rats>1) return Status::TooManyNonlinear;
                if(sampleRate<32000.f || sampleRate>96000.f)return Status::UnsupportedCmosRate;
                for(float control:{s.gemini.drive,s.gemini.volume})
                    if(!finite(control)||control<0||control>1)return Status::InvalidParameter;
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
        const Stage* ratStage = nullptr;const Stage* cmosStage=nullptr;
        for (size_t i = 0; i < patch.count; ++i)
            if (patch.stages[i].type == Type::Rat) ratStage = &patch.stages[i];
            else if(patch.stages[i].type==Type::GeminiCmos)cmosStage=&patch.stages[i];
        if (ratStage) {
            const bool existed = std::get_if<dsp::Rat>(&nonlinear_)!=nullptr;
            if (!existed) nonlinear_.emplace<dsp::Rat>(); // Fixed shared storage; control side only.
            // Rat::configure rejects without changing existing state. Nothing
            // after this successful configuration can reject the native commit.
            if (!std::get_if<dsp::Rat>(&nonlinear_)->configure(sampleRate_, ratStage->rat, {}, dsp::RatRate::Oversampled4x)) {
                if (!existed) nonlinear_.emplace<std::monostate>();
                return Status::InvalidParameter;
            }
        } else if(cmosStage) {
            // Small control-stack candidate; a failed DC preparation cannot destroy
            // the current alternative/history. No second persistent DSP allocation.
            dsp::Cmos prepared;
            if(!prepared.configure(sampleRate_,{cmosStage->gemini.drive,cmosStage->gemini.volume},
                dsp::geminiCircuit(cmosStage->gemini.biasLeak),dsp::CmosRate::Oversampled4x))return Status::InvalidParameter;
            nonlinear_.emplace<dsp::Cmos>(prepared);
        } else nonlinear_.emplace<std::monostate>();
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
                case Type::Rat:
                    // Explicit software convention: 1 engine unit = 1 model V.
                    // Rat returns bounded jack volts / 4; no extra normalization.
                    if(auto* rat=std::get_if<dsp::Rat>(&nonlinear_))x=rat->process(clip(x,-4.f,4.f));
                    else x=0;
                    break;
                case Type::GeminiCmos:
                    if(auto* cmos=std::get_if<dsp::Cmos>(&nonlinear_))x=cmos->process(clip(x,-2.f,2.f));
                    else x=0;
                    break;
            }
        }
        // Output limiting is a hard safety clamp, NOT a mastered level-management scheme.
        return clip(x * patch_.outputTrim * volume_, -1.f, 1.f);
    }
    const Patch& loaded() const noexcept { return patch_; }
    // Read-only same-thread diagnostics, including disabled-state regression.
    dsp::RatNodes ratNodes() const noexcept {const auto* r=std::get_if<dsp::Rat>(&nonlinear_);return r?r->lastNodes():dsp::RatNodes{};}
    dsp::CmosNodes cmosNodes() const noexcept {const auto* c=std::get_if<dsp::Cmos>(&nonlinear_);return c?c->lastNodes():dsp::CmosNodes{};}
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
    static_assert(sizeof(dsp::Cmos)<=sizeof(dsp::Rat),"Gemini must fit the existing nonlinear state budget");
    std::variant<std::monostate,dsp::Rat,dsp::Cmos> nonlinear_; // ONE alternative, not one per effect/stage.
};
} // namespace parable
