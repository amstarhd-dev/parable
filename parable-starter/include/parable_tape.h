#pragma once
// SF-006 standalone reduced magnetic medium, NOT a canonical Engine processor.
#include <cstddef>
#include <cstdint>

namespace parable::dsp {
struct TapeConfig {
    float loopSeconds=1.2f, delaySeconds=.3f;
    float eraseRetention=0, feedback=.5f, drive=4;
    float recordHz=4500, replayHz=6000, mix=.5f;
};
struct TapeProbe {
    float eraseBefore=0, eraseAfter=0, writeBefore=0, recordValue=0;
    float written=0, playMagnetization=0, replay=0;
    uint32_t writeCell=0, eraseCell=0, playCell=0, pass=0;
};
class Tape {
public:
    static constexpr double kEraseToWriteSeconds=.015;
    static constexpr float kWriteFraction=.6f;
    static constexpr float kFaultRetention=.85f;
    static constexpr uint32_t kMaximumCells=192000;
    Tape() noexcept = default;
    Tape(const Tape&)=delete;
    Tape& operator=(const Tape&)=delete;
    Tape(Tape&&)=delete;
    Tape& operator=(Tape&&)=delete;
    // Stopped/control side ONLY. Caller owns/aligned-initializes storage and must
    // keep it exclusive/alive until audio is joined/stopped. Invalid prepare is
    // atomic; successful prepare clears only the required cells, no allocation.
    static uint32_t requiredCells(float rate, const TapeConfig&) noexcept;
    bool prepare(float rate, const TapeConfig&, float* storage, size_t capacity) noexcept;
    void reset() noexcept; // O(N), stopped only; zero medium/filter/history.
    void stop() noexcept { running_=false; } // joined/stopped control only.
    float process(float input) noexcept; // mono normalized units; fixed O(1).
    TapeProbe probe() const noexcept { return probe_; } // same audio thread only.
    uint32_t cells() const noexcept { return cells_; }
    uint32_t delayCells() const noexcept { return delay_; }
    uint32_t eraseCells() const noexcept { return erase_; }
private:
    TapeConfig config_{};
    float* medium_=nullptr;
    uint32_t cells_=0, delay_=0, erase_=0, write_=0, pass_=0;
    float recordCoefficient_=0, replayCoefficient_=0, record_=0, replay_=0;
    bool running_=false;
    TapeProbe probe_{};
};
static_assert(sizeof(Tape)<=192,"Gate 0 standalone state ceiling");
} // namespace parable::dsp
