#pragma once
// Experimental host source lane. No sensor SDK, wire format or correspondence rules.
#include "parable_activation.h"
#include <array>
#include <atomic>
#include <cstdint>

namespace parable::living {
using Time = uint64_t; // microseconds; explicit source or monotonic replay/control domain.
inline constexpr Time kHorizon = 86400000000ull;
inline constexpr size_t kChannels = 16, kRecords = 256, kSnapshots = 32;
enum class Kind : uint8_t { HeartRate, Interval, HrvSummary, Ppg, Ecg, Respiration,
                            Motion, Eda, Temperature, Derived, Control };
enum class Unit : uint8_t { Bpm, Millisecond, Arbitrary, Microvolt, BreathsPerMinute,
                            MilliG, Microsiemens, Celsius, Normalized };
enum class Origin : uint8_t { Observed, Synthetic };
enum class Delivery : uint8_t { Live, Recorded };
enum class Validity : uint8_t { Missing, Valid, Invalid };
enum class Quality : uint8_t { Unknown, Good, Poor };
enum class Permission : uint8_t { Unknown, Granted, Revoked, Expired };
enum class Support : uint8_t { Unknown, No, Yes };
enum class Destination : uint8_t { None, PreGain, PostGain }; // TEST ONLY, not patch mappings.
enum class State : uint8_t { Missing, Fresh, Stale, Invalid, NoPermission, ClockReset };
enum class Code : uint8_t { Ok, Invalid, UnknownChannel, Conflict, NoPermission, Clock,
                            Order, Duplicate, Late, Stale, Full, Exhausted };
enum Flags : uint8_t { MeasurementKnown = 1, ConfidenceKnown = 2 };
struct Record {
    Time measured = 0, arrived = 0, sequence = 0;
    uint32_t source = 0, channel = 0, timebase = 0, epoch = 0;
    std::array<float,3> value{};
    float confidence = 0; // only meaningful with ConfidenceKnown.
    Kind kind = Kind::HeartRate;
    Unit unit = Unit::Bpm;
    Origin origin = Origin::Synthetic;
    Delivery delivery = Delivery::Recorded;
    Validity validity = Validity::Missing;
    Quality quality = Quality::Unknown;
    Permission permission = Permission::Unknown;
    uint8_t flags = 0;
};
// Explicit calibration: control = controlAnchor + (source-sourceAnchor)*scale.
// An adapter supplies this evidence; the scheduler does not estimate clock drift.
struct Clock {
    uint32_t timebase = 1, epoch = 1;
    Time sourceAnchor = 0, controlAnchor = 0, horizon = kHorizon, uncertainty = 0;
    double scale = 1;
};
struct Profile {
    uint32_t source = 0, channel = 0;
    Kind kind = Kind::HeartRate;
    Unit unit = Unit::Bpm;
    Origin origin = Origin::Synthetic;
    Delivery delivery = Delivery::Recorded;
    Destination destination = Destination::None;
    Support background = Support::Unknown; // declared capability, not observed proof.
    float acquisitionHz = 0, deliveryHz = 0; // zero = unknown/event-driven.
    Time timestampResolution = 0, expectedLatency = 0, ttl = 1000000;
    Time consentUntil = 0; // explicit monotonic session expiry; zero grants nothing.
    Clock clock{};
    Permission permission = Permission::Unknown;
};
struct View {
    Record last{}; // exact accepted facts, not synthesized/normalized sensor values.
    Time mapped = 0, age = 0;
    uint64_t accepted = 0, gaps = 0;
    bool present = false, ageKnown = false;
    State state = State::Missing;
};
struct Counters {
    uint64_t accepted = 0, rejected = 0, overflow = 0, snapshotFull = 0;
    size_t highWater = 0;
};
struct Snapshot {
    Time at = 0;
    std::array<Time,2> until{};
    std::array<float,2> normalized{.5f,1.f}; // neutral pre=2*.5, post=1.
    uint32_t epoch = 0;
    std::array<State,2> state{State::Missing,State::Missing};
};
// SPSC: publish/invalidate are control-only. AudioReader alone consumes.
class Lane {
public:
    Code publish(Snapshot snapshot) noexcept;
    Code invalidate() noexcept; // invalidates old epoch without touching reader-owned memory.
    void resetAfterJoin() noexcept; // external quiescence REQUIRED; epoch is never reset.
    uint32_t epoch() const noexcept { return epoch_.load(std::memory_order_acquire); }
private:
    friend class Audio;
    std::array<Snapshot,kSnapshots> queue_{};
    alignas(4) std::atomic<uint32_t> written_{0}, read_{0}, epoch_{1};
    Time lastPublished_ = 0;
    bool published_ = false;
};
class Scheduler {
public:
    using Observer = void(*)(void*, const Record&, Time mapped); // control only, ordered accepted facts.
    explicit Scheduler(Lane& lane) noexcept : lane_(lane) {}
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Code declare(const Profile& profile) noexcept;
    Code permission(uint32_t source, uint32_t channel, Permission, Time until) noexcept;
    Code resetClock(uint32_t source, const Clock&) noexcept; // all source channels; new epoch only.
    Code ingest(const Record&) noexcept;
    Code advance(Time watermark, Observer observer=nullptr, void* context=nullptr) noexcept;
    // Feed all due input BEFORE committing watermark. Observer gets every event;
    // it must not throw/reenter; scheduler stores only latest facts and bounded FIFO.
    View view(size_t index) const noexcept;
    const Counters& counters() const noexcept { return counts_; }
    size_t queued() const noexcept { return queued_; }
    size_t channels() const noexcept { return channels_; }
private:
    struct Channel {
        Profile profile{};
        View view{};
        Time lastArrival=0, lastMeasured=0, sequence=0;
        bool ordered=false, measured=false;
    };
    struct Item { Record record; Time mapped; size_t slot; };
    size_t find(uint32_t source,uint32_t channel) const noexcept;
    void flush(size_t slot) noexcept;
    Lane& lane_;
    std::array<Channel,kChannels> channel_{};
    std::array<Item,kRecords> queue_{};
    size_t channels_=0, queued_=0, head_=0;
    Time now_=0, lastArrival_=0;
    bool advanced_=false, ingested_=false;
    Counters counts_{};
};
struct AudioReceipt {
    activation::Receipt patch;
    Snapshot target{}; // audio-owned copy for diagnostics, never an Engine reference.
    std::array<float,2> gain{1,1};
    std::array<State,2> state{State::Missing,State::Missing};
    uint32_t epoch=0, consumed=0;
    bool timeError=false;
};
// TEST-ONLY wrapper; Runtime retains sole ownership of Engine/DSP history.
// One audio owner; reset/destruction require joined/quiescent audio.
class Audio {
public:
    Audio(Lane&, activation::Runtime&, uint32_t actualSampleRate) noexcept;
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;
    void resetAfterJoin() noexcept;
    AudioReceipt processBlock(const float* input,float* output,size_t frames,Time now) noexcept;
private:
    Lane& lane_;
    activation::Runtime& runtime_;
    std::array<float,activation::kMaxBlockFrames> scratch_{}, post_{};
    Snapshot target_{};
    std::array<float,2> gain_{1,1};
    float step_=0;
    uint32_t epoch_=0;
    Time last_=0;
    bool timed_=false;
};
} // namespace parable::living
