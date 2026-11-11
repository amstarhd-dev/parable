#pragma once
// Control-side interchange data only. Never call the codec from an audio callback.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

namespace parable::patch {
inline constexpr std::string_view kFormat = "parable.patch";
inline constexpr uint32_t kVersion = 1; // Separate from the DSP prototype's schema=2.
inline constexpr size_t kMaxProcessors = 8;
inline constexpr size_t kMaxDocumentBytes = 16384;
inline constexpr size_t kMaxDepth = 8;
inline constexpr size_t kMaxValues = 512;
inline constexpr size_t kMaxStringBytes = 64;

struct Bypass {};
struct Gain { float linear; };
struct OnePoleLowpass { float cutoffHz; };
struct Rat { float distortion, filter, volume; }; // Normalized model controls [0,1].
inline bool operator==(Bypass, Bypass) noexcept { return true; }
inline bool operator==(Gain a, Gain b) noexcept { return a.linear == b.linear; }
inline bool operator==(OnePoleLowpass a, OnePoleLowpass b) noexcept { return a.cutoffHz == b.cutoffHz; }
inline bool operator==(Rat a, Rat b) noexcept {
    return a.distortion == b.distortion && a.filter == b.filter && a.volume == b.volume;
}
struct Processor {
    uint32_t id = 0;
    bool enabled = false;
    std::variant<Bypass, Gain, OnePoleLowpass, Rat> settings;
};
inline bool operator==(const Processor& a, const Processor& b) noexcept {
    return a.id == b.id && a.enabled == b.enabled && a.settings == b.settings;
}

enum class ErrorCode {
    InvalidJson, LimitExceeded, DuplicateKey, UnknownField, MissingField, WrongType,
    UnsupportedFormat, UnsupportedVersion, UnsupportedProcessor, InvalidNumber,
    OutOfRange, InvalidUnit, DuplicateId, InvalidEndpoint, InvalidRouting
};
struct Error {
    ErrorCode code;
    std::string field; // "$" followed by JSON Pointer segments.
    std::string rule;
};

class Codec;
class ValidatedPatch {
public:
    float sampleRateHz() const noexcept { return rate_; }
    float outputTrimLinear() const noexcept { return trim_; }
    size_t count() const noexcept { return count_; }
    // Only the first count() entries are populated. Processors are sorted by ID;
    // serialOrder() lists their IDs in signal order, independent of that sorting.
    const std::array<Processor, kMaxProcessors>& processors() const noexcept { return processors_; }
    const std::array<uint32_t, kMaxProcessors>& serialOrder() const noexcept { return order_; }
    friend bool operator==(const ValidatedPatch& a, const ValidatedPatch& b) noexcept {
        return a.rate_ == b.rate_ && a.trim_ == b.trim_ && a.count_ == b.count_
            && a.processors_ == b.processors_ && a.order_ == b.order_;
    }
private:
    friend class Codec;
    ValidatedPatch(float rate, float trim, size_t count,
                   std::array<Processor, kMaxProcessors> processors,
                   std::array<uint32_t, kMaxProcessors> order)
        : rate_(rate), trim_(trim), count_(count), processors_(processors), order_(order) {}
    float rate_, trim_;
    size_t count_;
    std::array<Processor, kMaxProcessors> processors_;
    std::array<uint32_t, kMaxProcessors> order_;
};
using DecodeResult = std::variant<ValidatedPatch, Error>;
class Codec {
public:
    // Returns a new value or a field/rule error; never mutates existing validated data.
    // Allocations and exceptions from resource exhaustion belong on the control side.
    static DecodeResult decode(std::string_view document);
    static std::string encode(const ValidatedPatch& patch);
};
} // namespace parable::patch
