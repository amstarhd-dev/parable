# Prepared bundle activation — PAR-011

This implements [ADR-001's activation protocol](adr/ADR-001-patch-authority.md)
with the unchanged canonical Codec, native preparation bridge and serial Engine.
[Gate 0](adr/ADR-001-par011-gate0.md) preceded implementation.
No DSP equations, patch-v1 bytes, journal format, stored-slot semantics or firmware
adapter change. Living modulation, Volume/Register handoff, crossfade, state/tail
migration and physical audio acceptance are outside this increment.

## API and lifecycle

`activation::Runtime(actualSampleRate)` owns four fixed bundles. Allocate the
complete object on control (or give it persistent storage); do not construct it on
an IRQ stack. Constructor creates empty Engines without RAT configuration. There
are four pool entries regardless of the number of saved documents; entries are
not assigned to preset numbers. Canonical documents remain saved authority. The
bundle's typed document and configured Engine are derived copies. No canonical
text heap, alternative serializer, pointer graph or shared_ptr is stored in it.

Single serialized control owner:

1. Read a complete saved document, or receive a complete edit. `stage(document)`
   first reserves free -> staging, then uses Codec validation, target-rate equality,
   `preparation::prepare`, and inactive `Engine::load`. RAT table/configuration and
   history initialization happen here. Existing one-RAT and normalized-control
   limits apply, including disabled processors. No automatic sample-rate conversion.
2. Check `Result.code`. Decoder/preparation errors retain their structured field,
   rule and code; Engine failure retains native status. Allocation failure returns
   a fixed AllocationFailed result. Failed staging releases its reservation and
   never touches active/pending Engines or the journal. Exceptions from ordinary
   bounded allocation are handled; no resource exhaustion is tested on hardware.
3. `publish(token)` release-publishes ready staging to the single pending mailbox.
   Busy retains staging for retry/discard; a pending bundle is never overwritten.
   `discard(token)` only cancels the matching staging generation. At most one ready
   staging entry exists. `collect()` performs bounded ACK checking and reclamation;
   call it regularly to keep retirement capacity available. There is no wait loop
   in these methods. `inspect()` reports conservative control-side phases, not
   asynchronously sampled Engine histories. `document(token)` returns a typed
   **copy**, or nothing for a stale/free token; it exposes no Engine or reference.
4. At startup, with audio quiescent, call `startAfterJoin()`, then start/resume the
   reader. Before a first prepared publication is adopted, valid blocks are zero.
   Rate is fixed for this Runtime lifetime; change rate using a new quiescent
   Runtime, not a concurrent setter.

One non-reentrant audio reader calls `processBlock(input, output, frames)` once per
block. Valid spans (including in-place) are required for nonzero frames; blocks
are limited to 1,024 frames. Oversized/null nonzero spans reject before adoption
and report zero processed frames. Null/zero is a boundary-only call. The receipt
has a token, frame count, status and adoption flag, with no strings or allocation.
Input/output memory ownership remains the caller's responsibility.

Stop is separate from save/selection/activation. Control calls `requestStop()`;
`stopped()` is a single bounded poll, never a wait. At the next boundary audio
retires active and acknowledges stop, then writes zeros. A full retirement mailbox
**defers stop** and continues the same active Engine until control collects. A stop
requested mid-block is observed at the next boundary; it never splits a block.
Pending/staging work is retained until explicit cleanup.

Before `stopAfterJoin()`, start/restart, or destruction, the caller must establish
**external audio quiescence**: host thread joined, or future ISR disabled and
proved out of flight with the appropriate barrier. `stopAfterJoin()` transfers the
audio role to control, cancels staging/pending and reclaims all entries in a four-entry
scan. It also works if audio ceased without acknowledging, or never ran. A timeout
is NOT evidence of quiescence; never reclaim while a reader might still execute.
The host tests join real threads. No target ISR lifecycle adapter is supplied.

## Ownership and memory ordering

Four entries cover simultaneous active + pending + ready staging + retired. Each
moves free -> staging -> pending -> active -> retired -> free; discard/failed
prepare and quiescent cancellation are explicit alternate exits. Only audio
mutates active DSP histories. Control owns staging configuration and reclamation.
Control bookkeeping may lag handoff and conservatively retain ownership; it must
never grant reuse early. Every reclaim validates the exact token and phase.

Tokens are `uint32_t`: generation in the high 30 bits, index in the low two; zero
means none. Generations increase even across discarded/failed reservations and
joined restarts. They never wrap: exhaustion at 1,073,741,823 refuses further
reservations. Tokens are scoped to the lifetime of their Runtime object, not
persistent IDs and never saved-slot numbers or wire metadata.

- Control writes all inactive configuration, then stores pending token **release**.
  Audio loads it **acquire**, latches once at the boundary and uses a local Engine
  identity for every sample. No load/configure/parse or cleanup runs there.
- Audio's prior block has completed its final old-history read before it stores
  old token to retired **release**, then clears pending **release**. Control reads
  these mailboxes **acquire** before marking consumed/retired or reusing storage.
  Reconfiguration -> next release publication -> next audio acquire completes the
  happens-before chain for repeated pool reuse.
- Control checks mailbox empty before synchronizing consumption bookkeeping and
  replacing its pending-token record: audio can consume between control loads.
  A stop ACK may also retire that just-consumed token between those loads; its
  exact generation ACK proves both consumption and final-read completion.
- Control clears retired **release** only after reclamation. Audio checks capacity
  **acquire**; when full it defers adoption and renders the current Engine. No
  overwrite, wait, spin, destructor or error formatting on audio.

All four shared words (pending, retired, stop request, stopped ACK) are aligned
`atomic<uint32_t>`. Compile-time assertions require always-lock-free, 4-byte size
and >=4-byte alignment. Audio uses only loads/stores, no CAS/exchange or exclusive
retry. C++ single-control/single-reader and coherent ordinary-memory requirements
remain preconditions. This is not a multiproducer/multireader API. The test
scheduler's credits, sleeps, log atomics and reference Engine configuration are
**outside** the guarded production callback.

## Evidence, costs and limitations

See [exact commands/results](PAR011_EVIDENCE.md). Host Engine=11,048 bytes;
bundle=11,304; four-bundle pool=45,216; complete Runtime=45,288. Pinned ARM compile:
11,040 / 11,288 / 45,152 / **45,224** bytes respectively. The 16 shared atomic bytes
are included, not extra. Remaining Runtime metadata/alignment is 72 bytes on each
ABI. Typed document is 240 host / 236 ARM bytes, included per bundle. No full Engine
per stage or per stored document library.

The ARM audio object is 712 text / 0 data / 0 BSS bytes, control object 4,949 / 0 / 0
at pinned -O2, before linking dependencies. Zero object BSS does NOT make the pool
free: its 45,224 bytes must be placed by a future caller. Direct compiler frames:
audio 72 bytes, stage 592, Engine::load 304; not cumulative, IRQ or high-water
measurements. Callback undefined symbols are only RAT process and bounded memset;
compiled loads/stores use DMB, no atomic helper/exclusive retry. No firmware calls
this Runtime. Existing firmware still builds independently.

Codec DOM, control errors and temporary preparation allocate outside audio. Test
allocation traces count requests, not peak live heap; target heap/stack fitness is
unmeasured. Stress uses a single fresh reference Engine (~11 KiB) outside guarded
callbacks, 512 bytes input/output, 16,384-byte token-to-document map and fixed log
capacities (50,000 control + 30,000 audio records, 4,480,000 bytes on this host).
Post-run CSV assembly/sorting has additional host allocations; no production log
buffers exist. Four document strings/native fixtures and the temporary simulated
journal are host test data, not additional runtime Engines. Thread stacks and
sanitizer overhead are not MCU budgets.

Host timings separately record stage preparation and zero-frame boundary handoff,
including timer/guard overhead. They are neither ARM estimates nor worst-case
callback deadlines. Host guard interposes C++ new/delete, executable/static-object
C allocation, pthread lock/condition and selected read/write/stdio calls; it is
self-tested but is not a universal trace of shared-library internals or every OS
operation. Source and actual ARM call-site inspection complement it.

Switches reset histories and may click or cut tails. Publication alone proves no
smoothness, physical final level, CPU deadline or guitar quality. Real SRAM/stack
placement, physical M7/cache/ISR behavior, noise/latency and listening approval
remain unproven. The journal still uses synchronous Storage; no concurrent real
flash saving or assumption about pausing audio is introduced. No hardware was run.
