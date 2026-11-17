#pragma once
#include <cstddef>
#include <cstdint>
#include "../src/hall_core.h"
namespace parable::dsp {
struct HallSettings {
    float decay=1; // 0..1, normalized loop retention; measured decay in report
    float wet=.35f; // linear dry/wet crossfade, shared between both states
    float focusing=0; // 0 healthy; 0.98 damaged coherent rear reflecting plane
};
// Fixed caller-owned storage. Place in initialized SDRAM on target. No Engine
// registration. Same-owner use only; prepare/reset require stopped processing.
class Hall {
public:
    static constexpr size_t blockSize=64;
    bool prepare(float sampleRate, HallSettings settings={}) noexcept;
    void reset() noexcept; // stopped operation; clears energy, retains settings
    // Exactly64 mono input samples, stereo output. Input/output must not overlap.
    // Invalid entire block => zeros, unchanged history. Finite overload bounded.
    void process(const float* input,float* left,float* right) noexcept;
    uint32_t containmentHits() const noexcept { return hits_; }
    static constexpr size_t stateBytes() noexcept { return sizeof(hall_detail::Core); }
private:
    hall_detail::Core core_;
    HallSettings settings_{};
    uint32_t hits_=0;
    bool prepared_=false;
};
}
