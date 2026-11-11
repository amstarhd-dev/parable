# ADR-001 addendum — PAR-011 Gate 0, before implementation

Base main `c288eb196fc51f4b49627784e427e8c76be4c91c` includes merged PR #31.
Clean baseline: 20/20 Debug (94.51 s), 20/20 Release (23.28 s). Read the rules,
Constitution, status, patch contract/goldens, ADR-001 activation protocol, actual
Codec/prepare/Engine/Bank/Rat and PAR-010 journal. No root AGENTS.md; project rules
are in `parable-starter/AGENTS.md`. No architectural blocker found.

## Audit and prior art

Reuse decode -> typed validation -> preparation::prepare -> Engine::load, unchanged.
Engine reserves one optional RAT, not eight; Rat construction/configure builds a
2049-entry inverse table (56 iterations per entry) and FIR. This expensive work and
history reset belong only on inactive, control-owned Engines. Bank is RAM-only and
unsynchronized; journal documents remain the save authority. Neither is audio's
active pointer. No new serialization, graph engine or DSP kernel is warranted.

Inspected MIT [rigtorp SPSCQueue](https://github.com/rigtorp/SPSCQueue/blob/c8e9008dc4639220cc9bcf4c84abd2ff0d344aa5/include/rigtorp/SPSCQueue.h)
and its license: acquire/release producer/consumer indices, bounded try operations,
and lifetime ownership. Reuse the SPSC publication/reclamation pattern; a generic
allocator/queue, cached indices and spin-push are unnecessary for single-item
mailboxes. No source import. Unlike a naive double buffer, a token alone cannot
authorize reuse until audio releases its final read. Shared pointers risk cleanup
on audio; CAS/exchange on M7 can introduce exclusive-access retry loops.

[C++ ordering](https://eel.is/c++draft/atomics.order) and
[GCC atomic builtins](https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html)
support the intended happens-before edges; neither promises every atomic type is
lock-free. Require aligned `atomic<uint32_t>` with compile-time lock-free/size
assertions. The pinned xPack ARM GCC 14.2.1-1.1 Cortex-M7 audit compiles this exact
primitive: acquire is LDR/DMB; release DMB/STR, no helper or retry. Host size/alignment
4/4; ARM 4/4. This is compiler evidence, not ISR/board/cache-placement validation.

## Bounded ownership decision

Use four fixed bundles, independent of the four stored preset numbers: one active,
one pending, one ready staging, one retired awaiting control. Four are justified
by their simultaneous occupancy, not by storing four prepared presets. Each has
one Engine and a fixed typed document copy; no Engine per stage or per document
library. Host Engine=11,048 bytes, typed patch=240; ARM=11,040/236. Expect about
45 KiB for the pool, not 11 KiB; measure complete layouts after implementation.
Parsing/error allocation is control-side; no canonical text heap in the pool.

One serialized control writer and one non-reentrant audio reader. Control reserves
free -> staging before decode/prepare/configure, then release-publishes a token to
a one-item pending mailbox. A second publication returns Busy and retains staging.
At each bounded block audio acquire-latches once, uses the same Engine throughout,
and release-acknowledges the previous token through a one-item retired mailbox
after its last read. Full retirement defers adoption, never waits. Clear pending
only after adoption/retirement publication. Control acquire-observes consumption
and ACK, validates token/generation, then reclaims; no audio cleanup/destruction.
Control phase bookkeeping may conservatively lag handoff; it never grants reuse
early. Tokens pack pool index + monotonically increasing 30-bit generation; refuse
exhaustion, never wrap. Tokens are scoped to the Runtime object's lifetime.

Startup is stopped. A bounded stop request/ack works at block boundaries; polling
never waits. If audio has already ceased, external thread join/ISR-disable plus
quiescence must precede a distinct stopped cleanup that transfers the audio role
to control. It cancels pending/staging and reclaims safely without awaiting an ISR.
Start/restart and destruction likewise require external quiescence. No unsafe
timeout reclamation. These requirements are tested with joined host threads;
future ISR lifecycle/placement must be separately verified.

Tests will use actual journal documents and DSP/reference Engines, adverse inputs,
thread scheduling, generation/phase logs, callback allocation/lock/IO guards,
complete regressions and sanitizers. Keep Volume/Register setters inaccessible
cross-thread. First activation resets histories and can click/cut tails; no fade,
modulation lane, live flash writes, hardware adapter or physical performance claim.
