# ADR-001 addendum — PAR-008 Gate 0

Decision: **proceed with this bounded integration**, 2026-11-11, before source edits.
Base: `440a99221af7cc3f3b7123db327bb161306d4856` (current main, PR #19).
Required constitution, engineering/Soundforge rules, patch contract/codec,
preparation/core, RAT API/implementation/lab/tests/reports and pinned firmware
wiring/scripts were inspected; none is missing. Clean host baseline: Debug
16/16 (72.14 s), Release 16/16 (17.64 s), GCC 14.2.0, CMake 3.31.6.

## Reuse and compatibility

Reuse the serial `Engine`, stopped-audio `prepare`/`load`, ID-resolved canonical
codec and existing `dsp::Rat` unchanged. RAT's fixed `configure/process/reset`
API already fits the engine. A new processor framework, solver, dynamic dispatch
or graph would add scope without resolving an incompatibility. Retain the
PiRAT/Rodent BSD-2-Clause/MIT provenance/notices and existing regression hashes;
no new imported DSP. Issue #25 authorizes experimental product plumbing; it
does not approve ADR-003's still-unproven physical fidelity or musical quality.

Select an **additive processor-registry extension within format v1**: exact type
`Rat`, revision 1, three mandatory quantities `distortion`, `filter`, `volume`,
unit `normalized`, finite binary32 [0,1]. No circuit/fault fields or defaults.
Type/revision is the existing explicit capability discriminator. This changes
which types this implementation supports, not the meaning of any old document.
Old readers reject `Rat` as unsupported rather than play something different.
All prior v1 golden bytes, negative fixtures, root/utility semantics, unknown-field
policy, versions other than 1, stable IDs, canonical number rules and routing
remain unchanged. A root v2 would unnecessarily invalidate existing version-2
rejection fixtures and duplicate an otherwise identical container contract.
Future reinterpretation of a type, routing or numeric rules requires a new
revision/version; additive registration is not license for silent reinterpretation.
The canonical JSON remains authority; the native patch stays a derived execution
copy. No engine-only RAT type may bypass the canonical preparation path in host
evidence. The firmware's static native test patch is an explicit build-only probe,
not a second editable/save authority or a new transport.

## Levels, state and failure boundaries

Use **1 engine amplitude unit = 1 modeled input volt**. At the RAT boundary
explicitly saturate to [-4,4] model volts, matching `Rat::process`; retain its
existing bounded `jackVolts / 4` output as engine amplitude, with no extra gain.
Upstream gain and downstream filter/trim remain explicit ordered stages. The
existing final +/-1 clamp/master Volume and finite-input containment stay in
force. No RMS matching, automatic normalization or calibrated electrical-I/O
claim. Controls pass directly to the model: distortion/filter use its existing
pot law and volume is electrical wiper fraction. Use nominal `RatCircuit` and
fixed `Oversampled4x` (193-tap FIR, nominal delay 48 base samples plus circuit).
RAT patches require the model's 32–96 kHz range and exact document/target equality;
utility-only patches retain 8–192 kHz support.

One fixed-storage optional RAT state **per Engine**, constructed/configured only
on stopped control calls; no heap and no eightfold replication. Reject more than
one Rat declaration, **including disabled instances**, in codec, preparation and
native validation, with actionable errors. Preserve all eight utility stages and
four independently held patch configurations; no per-preset DSP-state bank.
Disabled RAT passes its input without calling `process` or advancing history.
Validate the whole candidate first; RAT's existing atomic configure then precedes
the infallible native commit. Rejected loads/preparations preserve the current
configuration and running histories. Successful stopped reload resets state;
live/clickless switching remains outside this ticket.

## Resource gate and evidence to collect

Existing ARM RAT: 10,752-byte instance, 6,452-byte standalone object text, no
object data/BSS (not its working RAM or full linked cost). Expect one roughly
11 KiB Engine state plus small per-stage settings; optional storage reserves this
RAM even in a utility-only Engine. Prior internal-flash image spans 74,008 of
131,072 bytes, leaving 57,064 bytes before integration. RAM/linker capacity does
not presently suggest a rewrite. Actual linked library/text/data/BSS and Engine
sizes must be measured, not inferred from the object. Callback cost, target
stack high-water, analog levels, bootloader operation and audio deadlines remain
unknown; bounded algorithm structure is not a performance measurement.

Collect Debug/Release and ASan/UBSan integration tests, old codec/RAT/CMOS/
germanium regressions, canonical-driven dry/RAT/composite/reversed WAVs with raw
levels and standalone consistency, and a pinned actual Cortex-M7 ELF containing
active Engine+RAT callback processing. Preserve a bypass startup target. Retain
maps, sections, static allocations, compiler stack frames, symbols/disassembly,
undefined-symbol checks and baseline deltas. Attempt BOOT_NONE; if it overflows,
record the exact failure and use only the previously audited upstream SRAM/QSPI
layout, without editing a memory map. No flashing or hardware timing claims.

**Stop rule:** incompatible schema semantics, excessive static capacity or a link
requiring more than a narrow target adaptation stops this ticket for a separate
decision. No such blocker was found at Gate 0. Hardware bring-up, persistence,
activation, controller/phone and correspondence work remain separate tickets.
