#include "parable_prepare.h"
#include "prepare_internal.h"
#include <utility>

namespace parable::preparation {
namespace {
Error error(ErrorCode code, std::string field, std::string rule,
            uint32_t id = 0, Status status = Status::Ok) {
    return {code, std::move(field), std::move(rule), id, status};
}
std::string location(uint32_t id) {
    return "document.processors[id=" + std::to_string(id) + "]";
}
} // namespace

Result detail::prepare(SourceView source, float actualSampleRate) {
    const auto targetStatus = validate(makeBypassPatch(), actualSampleRate);
    if (targetStatus != Status::Ok)
        return error(ErrorCode::InvalidTargetRate, "target.sampleRateHz",
                     "actual rate must be finite and within [8000,192000] Hz", 0, targetStatus);
    const auto sourceStatus = validate(makeBypassPatch(), source.sampleRate);
    if (sourceStatus != Status::Ok)
        return error(ErrorCode::InvalidSource, "document.sampleRateHz",
                     "source rate must pass native validation", 0, sourceStatus);
    if (source.sampleRate != actualSampleRate)
        return error(ErrorCode::SampleRateMismatch, "target.sampleRateHz",
                     "actual rate " + std::to_string(actualSampleRate) + " must exactly match document rate "
                     + std::to_string(source.sampleRate) + "; no conversion is supported");
    if (source.count > kMaxStages || source.count > source.processors.size() || source.count > source.order.size())
        return error(ErrorCode::CapacityExceeded, "document.count",
                     "processor count exceeds native capacity or available source entries");

    Patch prepared = makeBypassPatch(); // Internal schema=2, not the wire format's version=1.
    prepared.outputTrim = source.outputTrim;
    const auto trimStatus = validate(prepared, actualSampleRate);
    if (trimStatus != Status::Ok)
        return error(ErrorCode::NativeValidationFailed, "document.outputTrimLinear",
                     "output trim rejected by native validation", 0, trimStatus);
    for (size_t i = 0; i < source.count; ++i) {
        const uint32_t id = source.order[i];
        if (id == 0)
            return error(ErrorCode::UnresolvedInstance, "document.serialOrder[" + std::to_string(i) + "]",
                         "serial order requires a nonzero instance ID");
        for (size_t j = 0; j < i; ++j)
            if (source.order[j] == id)
                return error(ErrorCode::InvalidSource, "document.serialOrder[" + std::to_string(i) + "]",
                             "each instance must occur exactly once", id);
        const patch::Processor* processor = nullptr;
        for (size_t j = 0; j < source.count; ++j) {
            if (source.processors[j].id != id) continue;
            if (processor)
                return error(ErrorCode::InvalidSource, location(id) + ".id", "ambiguous duplicate instance ID", id);
            processor = &source.processors[j];
        }
        if (!processor)
            return error(ErrorCode::UnresolvedInstance, "document.serialOrder[" + std::to_string(i) + "]",
                         "instance ID has no processor in the source collection", id);
        Stage stage;
        stage.enabled = processor->enabled;
        const char* parameter = "parameters";
        if (std::get_if<patch::Bypass>(&processor->settings)) {
            stage.type = Type::Bypass;
            stage.parameter = 1.f; // Explicit neutral placeholder; Bypass has no wire parameter.
        } else if (const auto* gain = std::get_if<patch::Gain>(&processor->settings)) {
            stage.type = Type::Gain;
            stage.parameter = gain->linear;
            parameter = "gain.linear";
        } else if (const auto* lowpass = std::get_if<patch::OnePoleLowpass>(&processor->settings)) {
            stage.type = Type::OnePoleLowpass;
            stage.parameter = lowpass->cutoffHz;
            parameter = "cutoffHz";
        } else if (const auto* rat = std::get_if<patch::Rat>(&processor->settings)) {
            stage.type = Type::Rat;
            stage.rat = {rat->distortion, rat->filter, rat->volume};
            parameter = "rat.controls";
        } else {
            return error(ErrorCode::UnsupportedProcessor, location(id) + ".settings",
                         "no native mapping for these processor settings", id);
        }
        prepared.stages[i] = stage;
        prepared.count = static_cast<uint8_t>(i + 1);
        // Revalidate each prefix at the actual rate so a failure names its instance/parameter.
        const auto status = validate(prepared, actualSampleRate);
        if (status == Status::TooManyRats)
            return error(ErrorCode::CapacityExceeded, location(id) + ".settings",
                         "at most one Rat per patch, including disabled instances", id, status);
        if (status == Status::UnsupportedRatRate)
            return error(ErrorCode::NativeValidationFailed, "document.sampleRateHz",
                         "Rat requires [32000,96000] Hz at the actual target rate", id, status);
        if (status != Status::Ok)
            return error(ErrorCode::NativeValidationFailed, location(id) + "." + parameter,
                         "mapped stage rejected by native validation at the actual rate", id, status);
    }
    // The complete returned object, including the empty path, passes native validation.
    const auto status = validate(prepared, actualSampleRate);
    if (status != Status::Ok)
        return error(ErrorCode::NativeValidationFailed, "nativePatch", "prepared patch failed native validation", 0, status);
    return prepared;
}

Result prepare(const patch::ValidatedPatch& document, float actualSampleRate) {
    return detail::prepare({document.sampleRateHz(), document.outputTrimLinear(), document.count(),
                            document.processors(), document.serialOrder()}, actualSampleRate);
}
} // namespace parable::preparation
