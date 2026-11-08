# ADR-001: Patch authority

Status: **SELECTED — Option A, authorized by the founder for PAR-002 / issue #2.**
Date: 2026-11-08
Scope: canonical patch data, validation, preparation, activation and four-slot storage.

Decision amendment (PAR-002, separately reviewable): implement canonical versioned
JSON and validated typed control-side data first. The decision is selected;
this PR's implementation still requires review. Prepared runtime plans, activation
ownership and durable storage remain deferred. Their descriptions below record
the intended direction, not implemented capabilities.

## Context and decision boundary

Parable translates correspondence into playable sonic environments. Traditional
Western astrology is the first compiler; manual Workshop editing and future
biometric or other grammars must produce the same patch contract. The portable
C++ renderer must not infer sound from chart globals, editor widgets, transport
state or the hardware mounting arrangement.

At the PAR-001 baseline, `include/parable_core.h` had a provisional `schema=2`, eight ordered stages,
one anonymous float per stage and four RAM slots. `Bank::set` validates before
copying but provides no cross-thread synchronization. `Engine::load` copies the
patch, resets DSP state and calculates coefficients; it requires stopped audio.
That baseline had no codecs, durable presets, instance IDs or explicit graph edges.
The status report records the limited tests and remaining safety gaps.

This ADR evaluates exactly two approaches. Both keep one semantic playback
authority and separate portable DSP from board support. No correspondence rule,
new effect topology, transport choice or physical form is selected here. An
explicit routing representation does not authorize parallel/feedback effects.
The first implementation should accept only the existing supported serial path;
unsupported topology is rejected rather than silently simplified.

## Option A — Canonical JSON document, prepared native snapshot

The authoritative interchange/save representation is a versioned JSON patch
document. A portable control-side C++ decoder produces a typed, validated value
model; a preparation step derives a fixed-capacity runtime plan. That plan is a
cache of the document's semantics, never another editable patch authority.

- **Versioning:** use a format name and integer format version, plus processor
  type/revision identifiers. The current internal `schema=2` makes no wire
  compatibility promise and must not be relabeled as a supported saved format.
  Unsupported versions fail closed; explicit, tested migrations run off audio.
- **Identity and routing:** each processor has a stable patch-local instance ID
  separate from its type/revision. Two instances of the same processor have
  different IDs. Reordering preserves identity. Edges name source/destination
  instance IDs and ports, with reserved input/output endpoints. Reject duplicate
  IDs/edges, dangling references, incompatible ports, disconnected output,
  unsupported cycles and excess resource requirements. A deterministic execution
  order is derived from edges, not JSON object order or array indices.
- **Parameters and units:** a versioned processor descriptor registry defines
  parameter IDs, types, units, defaults and bounds shared by every compiler and
  manual editor. Use explicit physical units, such as `Hz` and linear amplitude;
  converting a UI percent to Hz is an explicit composer operation. Reject unknown
  fields/types/units, duplicate JSON keys, missing required values, non-finite or
  overflowing numbers and out-of-range values. Never clamp malformed saved data
  into a different patch. Sample-rate checks and measured target budgets run
  during preparation. For the current low-pass, the actual rule is cutoff >= 20
  Hz and < 0.45 * sample rate, not the header comment's fixed 20000 Hz maximum.
- **Numeric contract:** specify IEEE-754 binary32 playback values with explicit
  rounding at validation. Canonical writers emit enough decimal digits for an
  exact binary32 round trip, normalize negative zero, omit no playback fields,
  sort object keys/ID-addressed collections deterministically and use stable
  encoding. This normalization is part of the format, with golden fixtures;
  whitespace and original numeric spelling are not preserved. Integer IDs and
  versions have bounded exact ranges. Do not depend on a browser's default JSON
  formatting or raw C++ struct layout.
- **Snapshot:** runtime plan contains validated IDs resolved to bounded indices,
  ordered operations, parameters, prepared coefficients and resource accounting.
  Its configuration is immutable after publication. Filter histories and other
  mutable DSP state are separate, owned exclusively by audio while active.
- **Four presets:** slots 0..3 each own a complete normalized document and storage
  revision. None references another slot or mutable editor memory. Save/recall is
  separate from activation. The storage protocol below writes canonical UTF-8
  bytes; no struct padding, pointers or transient DSP state is persisted.
- **Round trip:** `decode(encode(validatedPatch)) == validatedPatch` for every
  playback field, identity, edge, mapping and supported metadata value.
  `encode(decode(canonicalBytes)) == canonicalBytes` for canonical inputs.
  Noncanonical accepted JSON normalizes on first write. An optional future binary
  transport is a lossless codec of the same value model with parity fixtures,
  not an independent schema or playback authority. It is not needed for pilot.
- **Multiple composers:** chart rules emit the document plus an auditable trace;
  Workshop edits that same document through validation. Optional bounded
  provenance carries compiler/rule versions and trace references but cannot
  override playback. Manual edits mark affected trace entries stale; a compiler
  cannot regenerate over them without an explicit user action. Runtime needs no
  chart, biometric sample or editor session after saving. Supported metadata
  survives round trips; unsupported versions/extensions are rejected explicitly.

Benefits: directly inspectable diffs/fixtures, natural phone integration, shared
manual/compiler editing, and no binary-to-JSON authoring translation initially.
Costs: larger payloads and parser footprint, a strict number/normalization
contract, and two representations (document and derived plan) requiring parity
tests. Parsing belongs outside audio and must have size/depth/count limits.
A parser dependency and its license would need review in an implementation ticket.

## Option B — Canonical binary records, JSON authoring projection

The authoritative saved/interchange patch is a compact binary record stream.
A portable C++ codec handles an explicit header (magic, major/minor version,
length), bounded instance/edge/parameter sections and integrity envelope. JSON
is a generated authoring/debug projection of these records; editors and compilers
must encode and revalidate through the binary contract before activation/save.

- **Versioning:** specify field widths, byte order (little-endian), record tags,
  count/length limits and a versioned processor registry. Never `memcpy` a native
  `Patch` into storage. Unknown required tags, versions, truncated records and
  conflicting lengths are errors. Migrations are explicit control-side steps.
- **Identity and routing:** bounded unsigned instance IDs and port IDs are
  distinct from processor type/revision tags. Edge records refer to IDs, never
  serialized memory pointers or current array positions. Apply the same graph
  and resource validation as Option A, then resolve IDs into native plan indices.
- **Parameters and units:** tagged parameter records carry descriptor-defined
  type/unit IDs and explicit binary32 or integer encodings. The same registry
  governs the JSON projection. Reject non-finite floats, incorrect units/types,
  duplicate IDs and invalid bounds at decode and target preparation. This avoids
  decimal ambiguity in canonical storage but adds a JS binary codec obligation.
- **Snapshot and activation:** decode into a bounded value model and prepare a
  separate immutable native plan, with audio-owned mutable DSP state. Use the
  common ownership protocol below; a compact serialized blob is not itself a
  safe runtime plan and cannot be patched in place while rendering.
- **Four presets:** each slot independently journals a complete binary image and
  storage revision using the same commit/recovery protocol below. No shared
  mutable stage arrays, inheritance or whole-bank rewrite to save one slot.
- **Round trip:** canonical record ordering, reserved-bit rules, explicit defaults
  and float encoding make `encode(decode(canonicalBytes)) == canonicalBytes`
  testable. `decode(encode(patch)) == patch` includes IDs, mappings and metadata.
  JSON export/import must recover exactly the same binary playback values and
  canonical bytes; golden cross-language tests are mandatory. Unknown records
  are rejected rather than silently stripped by the JSON projection.
- **Multiple composers:** all compilers/manual edits use the same registry and
  binary codec. Bounded provenance records preserve compiler/rule versions and
  trace references without granting them playback authority. Manual edits mark
  trace provenance stale just as in Option A. Phone access requires maintained
  binary bindings or a service performing the conversion; playback remains
  entirely standalone after saving.

Benefits: compact transfer/storage, predictable decode bounds, and exact stored
numeric encodings. Costs: a custom wire format and binary tooling immediately,
less readable review/diagnosis, and JSON/binary equivalence plus C++/JS bindings
before basic editing is trustworthy. Binary versioning and migration do not
remove the need for semantic validation or native preparation.

## Activation protocol required by either option

1. A single control-side owner receives a complete edit/compiler result. Decode,
   migrate if explicitly supported, validate, check target sample rate/resource
   profile, prepare coefficients, and initialize a new plan/state bundle outside
   the callback. Bound all input sizes and work. Preparation failure leaves the
   active bundle unchanged and returns a field-specific error to the composer.
2. Reserve a free bundle from a fixed pool before preparing. Distinct active,
   pending and staging/retired ownership is required; a naive two-buffer swap
   cannot overwrite a buffer audio still reads. If no bundle is available,
   return busy or defer on control; audio never waits. Publish at most one
   pending bundle with release semantics. Never mutate it after publication.
3. The callback performs only a bounded acquire/latch of an already prepared
   bundle at a block boundary. All parsing, graph edits, allocation and activation
   preparation happen outside it. The latch is the final atomic activation step,
   not permission to run `Engine::load` inside the callback. Only audio mutates
   the active bundle's DSP histories.
4. Audio acknowledges the retired bundle through a bounded ownership handoff
   after its last read. Control reclaims/reuses it only after that acknowledgement;
   no destructor/free or reference-count cleanup runs in audio. Generations
   prevent stale acknowledgements/ABA when slots are reused. Define a separate
   stopped-audio handshake for startup/shutdown so control cannot wait forever
   for a callback that is no longer running.
5. Require and test lock-free, aligned publication/acknowledgement primitives on
   the target. `std::atomic` is not automatically lock-free on all platforms.
   Platform synchronization belongs in an adapter if needed. No blocking locks,
   I/O, exceptions, network access, unbounded loops or hidden library allocation
   may enter the callback. Continuous parameter/Volume changes need an equally
   explicit control handoff; ordinary setters are not thread-safe by declaration.

The first switch policy may reset the new DSP state at the boundary; this can
click or cut tails. Clickless transitions and state migration need separate
measured CPU/memory and listening acceptance. Rejecting a patch must preserve
the prior active bundle; this alone does not prove bounded audio. Validate
levels/resources and test extreme finite input, NaN/Inf handling and internal
overflow before claiming safety. Current output clamping is not that proof.

## Durable four-slot protocol required by either option

Each slot has an independent generation history. Write a new image to an
inactive journal region with format version, slot ID, generation, payload
length and CRC. Read back and validate the payload, then write a storage-adapter
commit marker last; acknowledge Save only after durability is established.
On boot choose the newest complete, CRC-valid, semantically valid supported
generation for each slot. An interrupted save retains the previous committed
generation, and editing/clearing slot 0 must not alter slots 1..3. If no valid
generation exists, load an explicit safe default and report recovery status.
Do not silently replace unsupported newer formats as if a save had succeeded.

Generation wrap, erase alignment, wear, journal compaction and the commit marker's
atomic write guarantees need target-specific design and power-cut tests. A CRC
detects corruption; it does not make a flash write atomic. Flash erase/program
may stall code or interrupts even from control code; establish a hardware-safe
storage strategy (or stop audio explicitly) before concurrent saves are allowed.
Saving, selecting a slot, and activating a prepared bundle have separate result
states. A successful audition/activation does not mean the edit was saved.

## Selected decision and tradeoffs

Select **Option A**, following founder authorization in issue #2 and this assignment. Inspectable patch documents
fit phone-first composition, correspondence audit traces and manual editing
before storage/transfer efficiency has been measured as a bottleneck. Keep
JSON completely out of audio; bounded native plans preserve the portable C++
render path and permit target-specific budgets without encoding Seed GPIO,
enclosure or mounting decisions into patches.

The main tradeoff is parser/storage cost and disciplined numeric canonicalization.
Option B is viable if measured flash/RAM/transfer limits make JSON impractical,
but would add codec and authoring infrastructure now without evidence of that
need. Neither approach chooses a new DSP topology or changes astrology. The
existing eight-stage limit is a prototype bound, not a product capacity claim.
Schema maxima must be finite; target runtime limits must be backed by measurements.

## Increment boundary and follow-up

The representation choice is authorized. [`../PATCH_FORMAT_V1.md`](../PATCH_FORMAT_V1.md)
defines the first bounded contract: `parable.patch` version 1, three revision-1
utility types, stable uint32 IDs, a single mono serial path, explicit units and
binary32 numeric rules. It is separate from internal DSP `schema=2`. Named
input/output endpoints imply the sole mono ports; extra port/channel fields and
other topologies reject. Version 1 rejects metadata, provenance and expression
mappings rather than inventing incomplete rules for them. Future versions must
define those fields and any migration explicitly. The eight-processor capacity
is an interchange limit for this increment, not measured MCU capacity.

The codec uses pinned MIT-licensed nlohmann/json 3.11.3 for bounded JSON ingress,
then validates and returns immutable-access native data. Direct decimal-to-float
conversion and fixed nine-significant-digit scientific output avoid relying on
generic JSON float formatting. Payload/parser size, C++17 floating `charconv`
availability on the target, cross-language parity and measured memory still need
evidence. These costs could require a smaller parser or later lossless binary
codec; they do not justify a second editable playback authority. Storage and
activation remain separate hardware/ownership tickets.

First implementation ticket: **PAR-002 — Minimal canonical patch codec and validation
fixtures**. Cover only existing Bypass/Gain/OnePoleLowpass,
stable instance IDs and explicit serial routing; no new effects or compilers.
Acceptance: deterministic round trip and canonical bytes; duplicate/dangling
IDs, units, bounds, versions, unsupported topology and non-finite numbers reject
with precise errors; four distinct serialized slot payloads remain independent;
chart-like and manual fixtures produce equal validated playback data. Use a
host-only codec with no activation/storage integration. Subsequent tickets add
ownership exchange and the journal with concurrency/power-loss fault injection,
then prove them on Daisy. None is implemented in PAR-001.
