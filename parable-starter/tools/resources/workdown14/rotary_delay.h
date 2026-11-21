#pragma once
// Narrow fixed-wrap/linear-read adaptation of DaisySP DelayLine, MIT (2020
// Electrosmith). Notice/pin in third_party/rotary. Internal valid-delay contract.
#include <cstdint>
namespace parable::dsp::rotary_detail {
inline float read(const float* line,uint32_t current,double delay) noexcept {
    constexpr uint32_t size=384;
    const auto whole=static_cast<uint32_t>(delay);const double fraction=delay-whole;
    const auto recent=current>=whole?current-whole:current+size-whole;
    const auto older=recent==0?size-1:recent-1;
    return float((1-fraction)*line[recent]+fraction*line[older]);
}
} // namespace parable::dsp::rotary_detail
