#include "parable_presets.h"
#include "preset_crc.h"
#include "patch_json.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <vector>

namespace parable::presets {
namespace {
constexpr std::array<uint8_t, 8> magic = {'P','A','R','P','S','E','T',0};
constexpr size_t headerSize = 64;
constexpr uint32_t documentKind = 1, clearKind = 2;
bool power2(size_t n) { return n && !(n & (n - 1)); }
size_t rounded(size_t n, size_t alignment) { return (n + alignment - 1) & ~(alignment - 1); }
void put(uint8_t* p, uint64_t n, size_t bytes) {
    for (size_t i = 0; i < bytes; ++i) { p[i] = static_cast<uint8_t>(n); n >>= 8; }
}
uint64_t get(const uint8_t* p, size_t bytes) {
    uint64_t n = 0;
    for (size_t i = 0; i < bytes; ++i) n |= uint64_t{p[i]} << (8 * i);
    return n;
}
bool all(const uint8_t* p, size_t n, uint8_t value) { return std::all_of(p, p + n, [=](uint8_t b){return b == value;}); }
enum class State { Empty, Interrupted, Corrupt, Valid, Unsupported, Error };
struct Record {
    State state = State::Corrupt;
    uint64_t generation = 0;
    uint32_t kind = 0;
    std::string canonical;
};
bool unsupported(patch::ErrorCode code) {
    return code == patch::ErrorCode::UnsupportedFormat || code == patch::ErrorCode::UnsupportedVersion
        || code == patch::ErrorCode::UnsupportedProcessor;
}
// Codec intentionally reports its first error. A future schema may add/remove
// fields and therefore fail closed-object checks before its version is inspected.
// Probe only on failure, using the SAME bounded parser, to prevent downgrade.
bool futureSchema(std::string_view text) {
    try {
        const auto doc = patch::detail::parse(text);
        const auto& root = doc.root;
        if (!root.is_object()) return false;
        if (root.contains("version") && root["version"].is_number_integer() && root["version"] != patch::kVersion) return true;
        if (root.contains("format") && root["format"].is_string() && root["format"] != patch::kFormat) return true;
        if (root.contains("processors") && root["processors"].is_array()) {
            for (const auto& p : root["processors"]) {
                if (!p.is_object()) continue;
                if (p.contains("revision") && p["revision"].is_number_integer() && p["revision"] != 1) return true;
                if (p.contains("type") && p["type"].is_string() && p["type"] != "Bypass" && p["type"] != "Gain"
                    && p["type"] != "OnePoleLowpass" && p["type"] != "Rat") return true;
            }
        }
    } catch (const patch::Error&) {} // Malformed JSON is corruption, not a future schema.
    return false;
}
Record readRecord(Storage& storage, const Geometry& g, const Layout& l, size_t slot, size_t copy) {
    Record r;
    const size_t start = l.bank(slot, copy);
    std::array<uint8_t, headerSize> header{};
    std::array<uint8_t, 256> block{};
    if (storage.read(start, header.data(), header.size()) != Io::Ok
        || storage.read(start + l.bankBytes - g.programBytes, block.data(), g.programBytes) != Io::Ok) {
        r.state = State::Error; return r;
    }
    const bool committed = all(block.data(), g.programBytes, 0xa5);
    if (!committed) {
        r.state = State::Interrupted;
        if (all(header.data(), header.size(), 0xff) && all(block.data(), g.programBytes, 0xff)) {
            // Distinguish truly empty media from an interrupted erase with payload remnants.
            r.state = State::Empty;
            for (size_t offset = 0; offset < l.bankBytes; offset += block.size()) {
                const size_t count = std::min(block.size(), l.bankBytes - offset);
                if (storage.read(start + offset, block.data(), count) != Io::Ok) { r.state = State::Error; break; }
                if (!all(block.data(), count, 0xff)) { r.state = State::Interrupted; break; }
            }
        }
        return r;
    }
    if (!std::equal(magic.begin(), magic.end(), header.begin())
        || get(header.data()+60, 4) != detail::crc32(header.data(), 60)
        || get(header.data()+12, 4) != slot || get(header.data()+16, 8) == 0) return r;
    r.generation = get(header.data()+16, 8);
    // Stable prefix identifies a committed future envelope without interpreting its body.
    if (get(header.data()+8, 2) != 1) { r.state = State::Unsupported; return r; }
    const size_t length = static_cast<size_t>(get(header.data()+24, 4));
    r.kind = static_cast<uint32_t>(get(header.data()+32, 4));
    if (get(header.data()+10, 2) != headerSize || !all(header.data()+36, 24, 0)
        || length > patch::kMaxDocumentBytes) return r;
    if (r.kind != documentKind && r.kind != clearKind) { r.state = State::Unsupported; return r; }
    if ((r.kind == documentKind && length == 0) || (r.kind == clearKind && length != 0)) return r;
    r.canonical.resize(length);
    if (length && storage.read(start+l.headerBytes, r.canonical.data(), length) != Io::Ok) { r.state = State::Error; return r; }
    if (get(header.data()+28, 4) != detail::crc32(r.canonical.data(), length)) return r;
    if (r.kind == documentKind) {
        auto decoded = patch::Codec::decode(r.canonical);
        if (const auto* error = std::get_if<patch::Error>(&decoded)) {
            if (unsupported(error->code) || futureSchema(r.canonical)) r.state = State::Unsupported;
            return r;
        }
        if (patch::Codec::encode(std::get<patch::ValidatedPatch>(decoded)) != r.canonical) return r;
    }
    r.state = State::Valid;
    return r;
}
}
Layout layout(Geometry g, size_t base) noexcept {
    Layout l;
    if (!power2(g.programBytes) || g.programBytes > 256 || !power2(g.eraseBytes)
        || g.eraseBytes < g.programBytes || g.eraseBytes > 1024*1024 || base % g.eraseBytes) return l;
    l.base = base;
    l.headerBytes = rounded(headerSize, g.programBytes);
    l.bankBytes = rounded(l.headerBytes + rounded(patch::kMaxDocumentBytes, g.programBytes) + g.programBytes, g.eraseBytes);
    l.totalBytes = kSlots * 2 * l.bankBytes;
    l.status = base > g.capacity || l.totalBytes > g.capacity - base ? Status::StorageFull : Status::Loaded;
    return l;
}
bool Loaded::usable() const noexcept {
    return status == Status::Loaded || status == Status::Recovered || status == Status::DefaultEmpty
        || status == Status::DefaultInterrupted || status == Status::DefaultCorrupt || status == Status::DefaultCleared;
}
Journal::Journal(Storage& storage, const patch::ValidatedPatch& safeDefault, size_t base)
    : storage_(storage), geometry_(storage.geometry()), layout_(layout(geometry_, base)), default_(patch::Codec::encode(safeDefault)) {}
Loaded Journal::load(size_t slot) {
    Loaded result;
    if (slot >= kSlots) { result.status = Status::InvalidSlot; return result; }
    if (layout_.status != Status::Loaded) { result.status = layout_.status; return result; }
    const auto a = readRecord(storage_, geometry_, layout_, slot, 0);
    const auto b = readRecord(storage_, geometry_, layout_, slot, 1);
    if (a.state == State::Error || b.state == State::Error) { result.status = Status::StorageError; return result; }
    if (a.state == State::Unsupported || b.state == State::Unsupported) { result.status = Status::Unsupported; return result; }
    if (a.state == State::Valid && b.state == State::Valid && a.generation == b.generation
        && (a.kind != b.kind || a.canonical != b.canonical)) { result.status = Status::Conflict; return result; }
    for (size_t i = 0; i < 2; ++i) {
        const auto s = i == 0 ? a.state : b.state;
        if (s == State::Corrupt || s == State::Interrupted) result.badBanks |= uint8_t(1 << i);
    }
    if (a.state == State::Valid || b.state == State::Valid) {
        result.activeBank = a.state == State::Valid && (b.state != State::Valid || a.generation >= b.generation) ? 0 : 1;
        const auto& record = result.activeBank == 0 ? a : b;
        result.generation = record.generation;
        result.status = record.kind == clearKind ? Status::DefaultCleared : result.badBanks ? Status::Recovered : Status::Loaded;
        result.canonical = record.kind == clearKind ? default_ : record.canonical;
    } else {
        result.status = a.state == State::Corrupt || b.state == State::Corrupt ? Status::DefaultCorrupt
            : result.badBanks ? Status::DefaultInterrupted : Status::DefaultEmpty;
        result.canonical = default_;
    }
    return result;
}
Saved Journal::save(size_t slot, std::string_view text) {
    if (slot >= kSlots) return {Status::InvalidSlot};
    if (text.size() > patch::kMaxDocumentBytes) return {Status::DocumentTooLarge};
    auto decoded = patch::Codec::decode(text);
    if (!std::holds_alternative<patch::ValidatedPatch>(decoded)) return {Status::InvalidDocument};
    auto canonical = patch::Codec::encode(std::get<patch::ValidatedPatch>(decoded));
    if (canonical.size() > patch::kMaxDocumentBytes) return {Status::DocumentTooLarge};
    return write(slot, canonical, documentKind);
}
Saved Journal::clear(size_t slot) { return write(slot, {}, clearKind); }
Saved Journal::write(size_t slot, const std::string& canonical, uint32_t kind) {
    const auto prior = load(slot);
    if (!prior.usable()) return {prior.status};
    if (prior.generation == UINT64_MAX) return {Status::GenerationExhausted};
    const uint64_t generation = prior.generation + 1;
    const size_t copy = prior.activeBank == 0 ? 1 : 0;
    const size_t start = layout_.bank(slot, copy);
    // Allocate before touching media; nothing allocates after the commit starts.
    std::vector<uint8_t> image(layout_.headerBytes + rounded(canonical.size(), geometry_.programBytes), 0xff);
    auto* h = image.data();
    std::fill_n(h, headerSize, 0);
    std::copy(magic.begin(), magic.end(), h);
    put(h+8, 1, 2); put(h+10, headerSize, 2); put(h+12, slot, 4); put(h+16, generation, 8);
    put(h+24, canonical.size(), 4); put(h+28, detail::crc32(canonical.data(), canonical.size()), 4);
    put(h+32, kind, 4); put(h+60, detail::crc32(h, 60), 4);
    std::copy(canonical.begin(), canonical.end(), image.begin()+layout_.headerBytes);
    std::array<uint8_t, 256> block{};
    for (size_t off = 0; off < layout_.bankBytes; off += geometry_.eraseBytes) {
        const auto io = storage_.erase(start+off, geometry_.eraseBytes);
        if (io != Io::Ok) return {Status::StorageError, 0, io};
    }
    // Includes the old commit marker: do not trust a successful erase without readback.
    for (size_t off = 0; off < layout_.bankBytes; off += block.size()) {
        const size_t count = std::min(block.size(), layout_.bankBytes-off);
        const auto io = storage_.read(start+off, block.data(), count);
        if (io != Io::Ok) return {Status::StorageError, 0, io};
        if (!all(block.data(), count, 0xff)) return {Status::VerificationFailed};
    }
    for (size_t off = 0; off < image.size(); off += geometry_.programBytes) {
        const auto io = storage_.program(start+off, image.data()+off, geometry_.programBytes);
        if (io != Io::Ok) return {Status::StorageError, 0, io};
    }
    for (size_t off = 0; off < image.size(); off += block.size()) {
        const size_t count = std::min(block.size(), image.size()-off);
        const auto io = storage_.read(start+off, block.data(), count);
        if (io != Io::Ok) return {Status::StorageError, 0, io};
        if (!std::equal(block.begin(), block.begin()+count, image.begin()+off)) return {Status::VerificationFailed};
    }
    block.fill(0xa5);
    const size_t commit = start+layout_.bankBytes-geometry_.programBytes;
    auto io = storage_.program(commit, block.data(), geometry_.programBytes);
    if (io != Io::Ok) return {Status::CommitUncertain, 0, io};
    io = storage_.read(commit, block.data(), geometry_.programBytes);
    if (io != Io::Ok) return {Status::CommitUncertain, 0, io};
    if (!all(block.data(), geometry_.programBytes, 0xa5)) return {Status::CommitUncertain};
    return {Status::Saved, generation};
}
} // namespace parable::presets
