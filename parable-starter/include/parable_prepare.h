#pragma once
#include "parable_core.h"
#include "parable_patch.h"
#include <string>
#include <variant>

namespace parable::preparation {
enum class ErrorCode {
    InvalidTargetRate, SampleRateMismatch, CapacityExceeded, InvalidSource,
    UnresolvedInstance, UnsupportedProcessor, NativeValidationFailed
};
struct Error {
    ErrorCode code;
    std::string field; // Typed document/target location, not an original JSON array index.
    std::string rule;
    uint32_t instanceId = 0; // Zero when no processor is associated with the error.
    Status nativeStatus = Status::Ok; // A failed native validation's status, if applicable.
};
using Result = std::variant<Patch, Error>;

// Control side only: no JSON parsing or Engine mutation. Returns an owned copy.
// Rates must match exactly; no resampling or reinterpretation is performed.
// Engine::load may consume success ONLY with audio stopped. This is not live activation.
Result prepare(const patch::ValidatedPatch& document, float actualSampleRate);
} // namespace parable::preparation
