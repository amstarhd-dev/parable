# Host canonical preset journal — PAR-010

This implements ADR-001's **storage protocol logic on a host simulator**. It does
not make the Seed3 persist presets. No engine/codec/DSP contract, activation,
firmware image, correspondence rule, phone transport or hardware driver changes.
[Gate 0](PAR010_GATE0.md) records the audit before source edits.

## API and ownership

Link `parable_presets` for `include/parable_presets.h`; `parable_flash_sim` is a
separate host backend. A single serialized control owner supplies `Storage`, an
explicit validated safe default, and optionally an erase-aligned partition base.
Geometry is immutable for the store's lifetime. Read/program/erase are synchronous:
`Ok` means completed and durable, not queued/cached. Failure may modify a prefix,
but must not touch any other region. No individual multi-byte program is assumed
atomic. The backend must enforce 1→0 programming, FF erase and alignment. Readback
must see actual durable bytes. These assumptions need hardware verification.

`save(slot, json)` validates through the existing Codec, canonicalizes and owns the
complete document, then stages a new record. Invalid/oversized inputs touch no
media. `load(slot)` returns an owned canonical string and status, never a pointer
into UI, flash or DSP state. No selected slot is stored. `clear(slot)` commits a
separate tombstone; it neither selects nor activates anything and is not secure
erasure (the older bank retains a backup). Default sample rate/settings are
explicit caller policy; no resampling or native schema conversion occurs.

Check `Loaded::usable()` AND the recovery status. Empty/interrupted/corrupt media
returns the supplied default under distinct statuses; a valid tombstone returns
`DefaultCleared`. A surviving valid bank with a damaged/interrupted partner reports
`Recovered`. `badBanks` also reports this on a tombstone. Unreadable media,
unsupported committed records, conflicting equal generations, bad geometry and
insufficient capacity return **no usable document**. Unsupported records block
save/clear, even if apparently older: migration/reformat requires a future explicit
action. Equal generations with identical kind/document choose bank 0. CRC failure
rejects a record; it cannot reveal the intended version of corrupted bits.

Future stopped-audio boundary: load and check status on control; decode and prepare
the returned document for the exact actual sample rate; only with audio stopped
may the existing caller load Engine and check its status. This library never calls
Engine or runs in a callback. Selecting, saving, clearing and activating remain
separate. Live active/pending/retired bundle ownership is a different ticket.

## Envelope and bounded layout

All numbers are explicitly little-endian bytes, never native struct dumps.
Each slot has two fixed banks; there is one record per bank, no append allocator,
filesystem, cross-slot metadata, compaction or garbage collector.

| Header offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 8 | `PARPSET` followed by NUL |
| 8 | 2 | Journal major 1 (independent of patch version) |
| 10 | 2 | Header length 64 |
| 12 | 4 | Slot 0..3 |
| 16 | 8 | Generation 1..UINT64_MAX |
| 24 | 4 | Canonical payload length, at most 16,384 |
| 28 | 4 | Payload CRC32/ISO-HDLC (empty CRC=0) |
| 32 | 4 | Kind 1 document, 2 clear tombstone |
| 36 | 24 | Reserved zero |
| 60 | 4 | CRC32 over header bytes 0..59 |

Header area rounds 64 up to supplied program granularity; FF padding is not data.
Payload starts there. Reserve all 16,384 bytes, rounded up to program granularity.
The commit indicator occupies the **last complete program granule of the bank**,
all bytes A5; it never shares a granule with header/payload. Bank size rounds
header + full payload capacity + commit granule up to erase granularity. Slot banks
are adjacent but erase-independent. Unused bytes remain FF. Geometry profile:
power-of-two program 1..256 bytes, power-of-two erase >=program and <=1 MiB.
Other profiles explicitly reject; this limit is not a claim about Daisy geometry.

For erase=4,096/program=256, bank=20,480 bytes, each slot=40,960 bytes,
four-slot region=**163,840 bytes**. Logical document capacity is 65,536 bytes;
both generations reserve 131,072 payload bytes. Header/padding/commit/alignment
account for the remainder. Maximum staged image buffer is 16,640 bytes; verification
uses a 256-byte scratch array. Two records, returned owned strings, the supplied
default and Codec DOM can coexist; actual heap/stack high-water and MCU fit are
**not measured**. JSON allocation failures can propagate on control; no Save is
acknowledged if an exception prevents return. There is no DSP allocation change.

## Write, recovery and failures

Recover both banks first. Choose the inactive bank and generation+1. UINT64_MAX
never wraps: save/clear refuse without mutation. Erase each inactive erase unit,
verify the entire bank is FF (including any old marker), program the new envelope
and canonical bytes with program-aligned FF padding, then compare all staged bytes
on readback. Program the separate commit granule **last**, verify it, and only then
return `Saved`. The previously active bank is never modified during this save.

Recovery requires complete marker, valid magic/slot/generation/header CRC/bounds,
payload CRC, supported schema and byte-exact `encode(decode(payload))` parity.
Choose the highest supported valid generation. Unknown journal majors and kinds
block automatic downgrade. A failed Codec decode also probes its existing bounded
parser for future format/version/type/revision, because closed-object errors may
precede version checks. A future schema with added fields is therefore preserved.
CRC is accidental-corruption detection, neither authentication nor a mathematical
guarantee against every multi-bit corruption/collision.

Power can fail after the full marker is durable but before acknowledgment. Return
`CommitUncertain` if marker program/readback fails; after reboot either the old or
fully committed new value may be selected. A partially written marker never counts
as committed. A Save error before commit retains the previous active record;
losing an acknowledgment does not roll back an already durable commit.

The simulator owns a nonvolatile byte vector. `powerCycle()` clears injection and
operation counters, **retaining bytes and wear counts**; recreate Journal over it.
`cutAfter(N)` permits exactly N attempted mutating bytes (including FF→FF) then
latches power loss, including after an operation's final byte but before its
acknowledgment. Erase/program can be interrupted within a granule. `failOnCall(N)`
fails a deterministic read/program/erase boundary; `corrupt` flips explicit bits.
Optional trace records operation/offset/length/result. Erase attempts conservatively
consume the configured per-unit wear budget, including interrupted attempts.
Exhausted wear, insufficient space and read/verify failures fail closed. There is
no wear leveling or promise of indefinite saves.

## Hardware adapter gates — still open

Before a separate embedded adapter: verify actual erase/program granularity,
alignment, partial-program/erase behavior, durability and readback/cache rules;
reserve a non-overlapping QSPI region after loader/firmware assets; choose an
endurance/wear and bad-block policy; establish execute-from-QSPI versus write
restrictions, interrupt/code placement and whether audio must pause. Real power
cuts can produce non-prefix damage and analog behaviors absent from this model.
Do not infer real atomicity, endurance, SDRAM/QSPI safety, runtime CPU cost or
power-cycle persistence from host tests. No physical hardware or listening tests
were performed for this storage task.

Next ticket: host-simulated atomic four-preset runtime bundle activation using
ADR-001 active/pending/retired ownership. Keep hardware adaptation separate.
