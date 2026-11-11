#pragma once
// Host-only deterministic evidence backend; no real flash/endurance claims.
#include "parable_presets.h"
#include <limits>
#include <vector>
namespace parable::presets {
class SimulatedFlash final : public Storage {
public:
    enum class Operation { Read, Program, Erase };
    struct Event { Operation operation; size_t offset, bytes; Io result; };
    explicit SimulatedFlash(Geometry geometry, uint64_t eraseLimit = UINT64_MAX);
    Geometry geometry() const noexcept override { return geometry_; }
    Io read(size_t offset, void* output, size_t bytes) override;
    Io program(size_t offset, const void* input, size_t bytes) override;
    Io erase(size_t offset, size_t bytes) override;
    // Every attempted mutating byte counts, including FF->FF. Cut can occur
    // after the last byte of an operation, before its acknowledgment.
    void cutAfter(size_t mutatingBytes) noexcept { remaining_ = mutatingBytes; }
    void failOnCall(size_t zeroBasedCall) noexcept { failCall_ = zeroBasedCall; }
    void powerCycle() noexcept; // Clears faults/counters/trace; retains bytes AND wear.
    bool corrupt(size_t offset, uint8_t xorMask) noexcept;
    const std::vector<uint8_t>& bytes() const noexcept { return bytes_; }
    const std::vector<uint64_t>& erases() const noexcept { return erases_; }
    size_t calls() const noexcept { return calls_; }
    size_t mutations() const noexcept { return mutations_; }
    void trace(bool enabled) { trace_ = enabled; events_.clear(); }
    const std::vector<Event>& events() const noexcept { return events_; }
private:
    Io check(size_t offset, size_t bytes);
    Io finish(Operation op, size_t offset, size_t bytes, Io result);
    Geometry geometry_;
    uint64_t eraseLimit_;
    std::vector<uint8_t> bytes_;
    std::vector<uint64_t> erases_;
    size_t remaining_ = SIZE_MAX, failCall_ = SIZE_MAX, calls_ = 0, mutations_ = 0;
    bool powered_ = true, trace_ = false;
    std::vector<Event> events_;
};
}
