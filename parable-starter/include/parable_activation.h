#pragma once
// One control owner, one non-reentrant audio reader. See ADR-001 PAR-011 addendum.
#include "parable_prepare.h"
#include <atomic>
#include <optional>

namespace parable::activation {
inline constexpr size_t kBundles = 4, kMaxBlockFrames = 1024;
using Token = uint32_t; // zero = none; generation << 2 | pool index, never a preset number.
inline constexpr uint32_t kMaxGeneration = UINT32_MAX >> 2;
constexpr size_t index(Token t) noexcept { return t & 3u; }
constexpr uint32_t generation(Token t) noexcept { return t >> 2; }
namespace detail {
inline Token issue(uint32_t& next, size_t slot) noexcept {
    if (!next || next > kMaxGeneration || slot >= kBundles) return 0;
    return (next++ << 2) | static_cast<uint32_t>(slot);
}
}
static_assert(std::atomic<uint32_t>::is_always_lock_free, "Activation requires lock-free uint32 loads/stores");
static_assert(sizeof(std::atomic<uint32_t>) == 4 && alignof(std::atomic<uint32_t>) >= 4);
enum class Phase { Free, Staging, Pending, Active, Retired };
enum class Code { Ok, Busy, InvalidToken, Stopping, NotStopped, GenerationExhausted,
                  InvalidDocument, PreparationFailed, EngineFailed, AllocationFailed, ProtocolFault };
struct Result {
    Result(Code c = Code::Ok, Token t = 0) : code(c), token(t) {}
    Code code = Code::Ok;
    Token token = 0;
    std::optional<std::variant<patch::Error, preparation::Error>> error;
    parable::Status nativeStatus = parable::Status::Ok;
};
enum class BlockCode { Rendered, NoActive, Deferred, Stopped, StopDeferred, InvalidBlock, ProtocolFault };
struct Receipt { BlockCode code; Token token; size_t frames; bool adopted; };
struct Slot { Phase phase = Phase::Free; Token token = 0; };
struct Snapshot { Code code; std::array<Slot, kBundles> slots; Token staging, pending, retired; };
class Runtime {
public:
    explicit Runtime(float actualSampleRate) noexcept;
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    // Control only. Reserve before decode/prepare/configure. One ready staging slot.
    Result stage(std::string_view completeDocument);
    Result publish(Token ready); // Busy retains staging; never overwrites pending.
    Result discard(Token ready);
    Result collect(); // Bounded ACK validation/reclamation, never waits.
    Snapshot inspect(); // Conservative control-side phases; no Engine/history reads.
    std::optional<patch::ValidatedPatch> document(Token token) const;
    // Control: start, stopped cleanup and destruction REQUIRE joined/quiescent audio.
    Result startAfterJoin() noexcept;
    void requestStop() noexcept { stop_.store(1, std::memory_order_release); }
    bool stopped() const noexcept { return stopped_.load(std::memory_order_acquire) != 0; }
    void stopAfterJoin() noexcept; // Also handles an ISR/thread that never ran; cancels queued work.
    // Audio only: complete bounded block, one Engine identity for every sample.
    // Valid spans (including in-place) required for frames>0. nullptr/0 is a boundary-only call.
    Receipt processBlock(const float* input, float* output, size_t frames) noexcept;
    static constexpr size_t bundleBytes() noexcept { return sizeof(Bundle); }
private:
    struct Bundle {
        explicit Bundle(float rate) noexcept : engine(rate) {}
        Engine engine;
        std::optional<patch::ValidatedPatch> document;
        Token token = 0; // Immutable from publication through ACK.
    };
    Code synchronize() noexcept;
    Code acknowledge(Token token) noexcept;
    void release(size_t slot) noexcept;
    float rate_;
    std::array<Bundle, kBundles> bundles_;
    std::array<Slot, kBundles> slots_{}; // Control only; may lag audio ownership conservatively.
    Token staging_ = 0, controlPending_ = 0;
    uint32_t nextGeneration_ = 1;
    alignas(4) std::atomic<uint32_t> pending_{0}, retired_{0}, stop_{1}, stopped_{1};
    Token active_ = 0; // Audio only; role transfers to control ONLY after external join.
};
} // namespace parable::activation
