#include "parable_flash_sim.h"
#include <algorithm>
#include <cstring>
namespace parable::presets {
SimulatedFlash::SimulatedFlash(Geometry geometry, uint64_t eraseLimit)
    : geometry_(geometry), eraseLimit_(eraseLimit), bytes_(geometry.capacity, 0xff),
      erases_(geometry.eraseBytes ? geometry.capacity / geometry.eraseBytes : 0, 0) {}
void SimulatedFlash::powerCycle() noexcept {
    remaining_ = failCall_ = SIZE_MAX;
    calls_ = mutations_ = 0;
    powered_ = true;
    events_.clear();
}
bool SimulatedFlash::corrupt(size_t offset, uint8_t mask) noexcept {
    if (offset >= bytes_.size()) return false;
    bytes_[offset] ^= mask; return true;
}
Io SimulatedFlash::check(size_t offset, size_t bytes) {
    const size_t call = calls_++;
    if (!powered_) return Io::PowerLost;
    if (call == failCall_) return Io::Failure;
    if (offset > bytes_.size() || bytes > bytes_.size()-offset) return Io::Bounds;
    return Io::Ok;
}
Io SimulatedFlash::finish(Operation op, size_t offset, size_t bytes, Io result) {
    if (trace_) events_.push_back({op, offset, bytes, result});
    return result;
}
Io SimulatedFlash::read(size_t offset, void* output, size_t bytes) {
    const auto io = check(offset, bytes);
    if (io == Io::Ok && bytes) std::memcpy(output, bytes_.data()+offset, bytes);
    return finish(Operation::Read, offset, bytes, io);
}
Io SimulatedFlash::program(size_t offset, const void* input, size_t bytes) {
    auto io = check(offset, bytes);
    if (io == Io::Ok && (!geometry_.programBytes || offset % geometry_.programBytes || bytes % geometry_.programBytes)) io = Io::Alignment;
    const auto* p = static_cast<const uint8_t*>(input);
    if (io == Io::Ok) {
        for (size_t i = 0; i < bytes; ++i)
            if ((bytes_[offset+i] & p[i]) != p[i]) { io = Io::ProgramViolation; break; }
    }
    if (io == Io::Ok) {
        const size_t count = std::min(remaining_, bytes);
        for (size_t i = 0; i < count; ++i) bytes_[offset+i] &= p[i];
        mutations_ += count;
        if (remaining_ != SIZE_MAX) {
            remaining_ -= count;
            if (!remaining_) { powered_ = false; io = Io::PowerLost; }
        }
    }
    return finish(Operation::Program, offset, bytes, io);
}
Io SimulatedFlash::erase(size_t offset, size_t bytes) {
    auto io = check(offset, bytes);
    if (io == Io::Ok && (!geometry_.eraseBytes || offset % geometry_.eraseBytes || bytes % geometry_.eraseBytes)) io = Io::Alignment;
    if (io == Io::Ok) {
        for (size_t i = 0; i < bytes; i += geometry_.eraseBytes)
            if (erases_[(offset+i)/geometry_.eraseBytes] >= eraseLimit_) { io = Io::WornOut; break; }
    }
    if (io == Io::Ok) {
        const size_t count = std::min(remaining_, bytes);
        // Conservatively count an attempted erase, including interrupted attempts.
        for (size_t i = 0; i < bytes && i <= count; i += geometry_.eraseBytes)
            ++erases_[(offset+i)/geometry_.eraseBytes];
        std::fill_n(bytes_.begin()+offset, count, 0xff);
        mutations_ += count;
        if (remaining_ != SIZE_MAX) {
            remaining_ -= count;
            if (!remaining_) { powered_ = false; io = Io::PowerLost; }
        }
    }
    return finish(Operation::Erase, offset, bytes, io);
}
}
