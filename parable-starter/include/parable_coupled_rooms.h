#pragma once
#include <cstddef>

namespace parable::soundforge {
struct RoomControls {
    float aperture = .35f, wet = .65f;
    bool invertedReturn = false;
};
struct RoomOutput { float mono = 0, a = 0, b = 0; };

// Standalone reduced acoustics; one owner, no concurrent calls. Storage remains
// caller-owned and valid until stopped reprepare/destruction. Never copy a live
// instance and share its buffer. prepare/reset require stopped processing.
class CoupledRooms {
public:
    static constexpr std::size_t kMaxSamples = 22000;
    static std::size_t requiredSamples(float rate) noexcept;
    bool prepare(float rate, float* storage, std::size_t count,
                 RoomControls controls = {}) noexcept;
    void reset() noexcept;
    // Audio-owner request, smoothly applied over about20ms; no graph mutation.
    bool setControls(RoomControls controls) noexcept;
    RoomOutput process(float inputA, float inputB = 0) noexcept;
    std::size_t bufferBytes() const noexcept { return count_*sizeof(float); }
    unsigned length(unsigned line) const noexcept { return line < 8 ? lengths_[line] : 0; }
private:
    static bool valid(RoomControls controls) noexcept;
    float* data_ = nullptr;
    std::size_t count_ = 0;
    unsigned offsets_[8]{}, lengths_[8]{}, positions_[8]{};
    float losses_[8]{}, damping_[8]{};
    float poles_[2]{}, smooth_ = 0;
    float aperture_ = 0, polarity_ = 1, wet_ = 0;
    RoomControls target_{};
};
} // namespace parable::soundforge
