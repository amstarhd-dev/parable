#pragma once
// First-boot integer accounting only; no DSP, peripheral access or reporting here.
#include <atomic>
#include <cmath>
#include <cstdint>

namespace parable::first_boot {
constexpr uint32_t kCaptureBlocks = 30000;
constexpr uint32_t kAmbiguousCycles = uint32_t{1} << 31;
struct Budget {
    uint32_t coreHz = 0, rateMilliHz = 0, frames = 0, deadlineCycles = 0;
};
// Control side, before starting audio. Clock comes from RCC/CMSIS readback.
inline bool makeBudget(uint32_t coreHz, float rate, uint32_t frames, Budget& out) noexcept {
    if (!coreHz || !std::isfinite(rate) || rate < 8000 || rate > 192000 || !frames || frames > 128)
        return false;
    const auto milliHz = static_cast<uint32_t>(double(rate) * 1000. + .5);
    const uint64_t cycles = uint64_t(coreHz) * frames * 1000 / milliHz;
    if (!cycles || cycles >= kAmbiguousCycles) return false;
    out = {coreHz, milliHz, frames, static_cast<uint32_t>(cycles)};
    return true;
}
inline uint32_t elapsedCycles(uint32_t start, uint32_t end) noexcept { return end - start; }
// Independent millisecond ticks give a conservative wrap guard for stopped load.
// Valid only outside the callback, with SysTick running throughout preparation.
inline bool startupIntervalValid(uint32_t coreHz, uint32_t elapsedMs) noexcept {
    return coreHz && (uint64_t(elapsedMs) + 1) * coreHz < (uint64_t{1} << 32) * 1000;
}
inline uint64_t ratioPpm(uint32_t cycles, uint32_t deadline) noexcept {
    return deadline ? uint64_t(cycles) * 1000000 / deadline : 0;
}
struct Summary {
    uint32_t blocks = 0, validBlocks = 0, maxCycles = 0, lastCycles = 0;
    uint32_t atOrAboveDeadline = 0, invalidIntervals = 0, frameMismatches = 0;
};
// One ISR writer, one control reader AFTER snapshot() succeeds. No mutable/live
// snapshot protocol: freeze after a fixed prefix and never write it again.
class Capture {
public:
    static_assert(std::atomic<bool>::is_always_lock_free, "ISR completion store must be lock-free");
    bool initialize(Budget budget, bool counterAvailable, uint32_t limit = kCaptureBlocks) noexcept {
        if (!limit || !budget.deadlineCycles || budget.deadlineCycles >= kAmbiguousCycles) return false;
        budget_ = budget; available_ = counterAvailable; limit_ = limit; summary_ = {};
        published_.store(false, std::memory_order_relaxed);
        return true;
    }
    // Callback writer only. Unsigned subtraction handles a counter wrap provided
    // each interval is <2^31 cycles. Multi-wrap hangs cannot be reconstructed.
    bool recording() const noexcept { return summary_.blocks < limit_; }
    void observe(uint32_t start, uint32_t end, uint32_t frames) noexcept {
        if (!recording()) return;
        ++summary_.blocks; // <= limit, so none of these counters can wrap.
        const auto cycles = elapsedCycles(start, end);
        if (frames != budget_.frames) ++summary_.frameMismatches;
        if (!available_ || !cycles || cycles >= kAmbiguousCycles || frames != budget_.frames) {
            ++summary_.invalidIntervals;
        } else {
            ++summary_.validBlocks;
            summary_.lastCycles = cycles;
            if (cycles > summary_.maxCycles) summary_.maxCycles = cycles;
            if (cycles >= budget_.deadlineCycles) ++summary_.atOrAboveDeadline;
        }
        if (!recording()) published_.store(true, std::memory_order_release);
    }
    const Summary* snapshot() const noexcept {
        return published_.load(std::memory_order_acquire) ? &summary_ : nullptr;
    }
private:
    Budget budget_{};
    Summary summary_{};
    uint32_t limit_ = 0;
    bool available_ = false;
    std::atomic<bool> published_{false};
};

enum class LedStatus : uint8_t {
    Initializing = 0, LedReady = 1, BypassReady = 2, RatReady = 3,
    WrongBoard = 4, InvalidConfiguration = 5, LoadFailed = 6,
    AudioStartFailed = 7, CaptureTimeout = 8, CounterUnavailable = 9
};
// Control side: N short pulses, then a clear pause, in each 3-second period.
inline bool statusLed(LedStatus status, uint32_t ms) noexcept {
    const auto pulses = static_cast<uint32_t>(status);
    if (!pulses || pulses > 9) return true;
    const uint32_t phase = ms % 3000;
    return phase < pulses * 200 && phase % 200 < 100;
}
} // namespace parable::first_boot
