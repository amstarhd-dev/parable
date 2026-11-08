#pragma once
#include "parable_prepare.h"

namespace parable::preparation::detail {
// Private, borrowed for the duration of preparation only. Allows guard testing
// without weakening ValidatedPatch's construction/immutability contract.
struct SourceView {
    float sampleRate, outputTrim;
    size_t count;
    const std::array<patch::Processor, patch::kMaxProcessors>& processors;
    const std::array<uint32_t, patch::kMaxProcessors>& order;
};
Result prepare(SourceView source, float actualSampleRate);
} // namespace parable::preparation::detail
