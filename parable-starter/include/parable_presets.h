#pragma once
// Serialized control-side owner only. No callback calls, Engine mutation or selection.
#include "parable_patch.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace parable::presets {
constexpr size_t kSlots = 4;
struct Geometry { size_t capacity, eraseBytes, programBytes; };
enum class Io { Ok, Bounds, Alignment, ProgramViolation, PowerLost, WornOut, Failure };
class Storage {
public:
    virtual ~Storage() = default;
    virtual Geometry geometry() const noexcept = 0;
    // Synchronous and durable on Ok. Failed operations may have changed a prefix.
    // Program sets only 1->0 bits. Erase restores FF. No multi-byte atomicity.
    // An operation must never affect bytes outside its addressed region.
    virtual Io read(size_t offset, void* output, size_t bytes) = 0;
    virtual Io program(size_t offset, const void* input, size_t bytes) = 0;
    virtual Io erase(size_t offset, size_t bytes) = 0;
};
enum class Status {
    Loaded, Recovered, DefaultEmpty, DefaultInterrupted, DefaultCorrupt, DefaultCleared,
    Saved, InvalidSlot, InvalidDocument, DocumentTooLarge, InvalidGeometry, StorageFull,
    StorageError, Unsupported, Conflict, GenerationExhausted, VerificationFailed, CommitUncertain
};
struct Layout {
    Status status = Status::InvalidGeometry;
    size_t base = 0, headerBytes = 0, bankBytes = 0, totalBytes = 0;
    size_t bank(size_t slot, size_t copy) const noexcept { return base + (2 * slot + copy) * bankBytes; }
};
// Supported profile: power-of-two program 1..256, erase program..1MiB.
Layout layout(Geometry geometry, size_t base = 0) noexcept;
struct Loaded {
    Status status = Status::StorageError;
    uint64_t generation = 0;
    std::string canonical; // Owned, empty on unusable status. Default is explicit.
    uint8_t badBanks = 0;
    int activeBank = -1; // Diagnostic only, not a selection/activation authority.
    bool usable() const noexcept;
};
struct Saved {
    Status status;
    uint64_t generation = 0;
    Io io = Io::Ok;
};
class Journal {
public:
    // Default is explicitly provided/validated by caller, not an implicit playback rate.
    Journal(Storage& storage, const patch::ValidatedPatch& safeDefault, size_t base = 0);
    const Layout& storageLayout() const noexcept { return layout_; }
    Loaded load(size_t slot);
    Saved save(size_t slot, std::string_view document);
    Saved clear(size_t slot); // Logical tombstone; not secure erase, save or activation.
private:
    Saved write(size_t slot, const std::string& canonical, uint32_t kind);
    Storage& storage_;
    Geometry geometry_;
    Layout layout_;
    std::string default_;
};
} // namespace parable::presets
