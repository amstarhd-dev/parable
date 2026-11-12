# SF-007 Gate 0 — before native implementation

2026-11-12, main **fadcb99be09222275468c7913e2b580107b12729**, approved #44 merge.
#44 is merged; #42 remains open (issue flags its merge conflict). Neither PR is
merged, cherry-picked, resolved or rewritten by this task. PAR-012 is already
merged and is regression-only. Read project AGENTS/Constitution/README/status,
ADR-001/PAR-008/PAR-011, ADR-006, canonical/native constraints, Soundforge recipe
and ARM reports. Frozen Lexicon and root AGENTS.md are not tracked; issue #45
authorizes Moon/Leo's sound target, **not affliction activation semantics**.

Fresh GNU14.2 / CMake3.31.6 baseline, repository root:

```sh
cmake -S parable-starter -B /workspace/work/sf007-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/sf007-Debug -j2
ctest --test-dir /workspace/work/sf007-Debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /workspace/work/sf007-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/sf007-Release -j2
ctest --test-dir /workspace/work/sf007-Release --output-on-failure --no-tests=error -j2
```

**28/28 each**, Debug128.13 s / Release74.31 s, before code edits. Capture current
canonical/living/tape bytes and existing protected sources before implementation.

## Borrow / adapt / original gap

Pinned MIT `cwbudde/algo-dsp` **455d4a569936db8e74edbf204476a78258deed78**,
`dsp/effects/modulation/rotary_speaker.go` and root LICENSE inspected. Adapt its
two-band 800 Hz split, nominal .8/6.5 Hz horn and .6/5 Hz drum, angle-dependent
directivity, Doppler delay and circular history; mono fixed-memory native port.
Don't import Go/framework, tanh drive, stereo/reflection voices, variable heap
buffers or first-order speed LFO. Its mechanical model is a time-constant ramp,
not belt/motor physics. This is a source-declared MIT grant, not authenticated
physical calibration or an independent audit of the author's ownership.

LeslieSim c82211a42a14a40914be8c857b2f69f69f3d97e0 evaluated; root LICENSE not
retrieved, no source imported. DAFx2002 Abel/Berners/Smith/Serafin and Smith's
rotating-horn text support Doppler/directivity; Modelica Clutch documentation
supports stick/slip torque balance and recapture. References only, not code or
measured Leslie motor parameters. Existing DaisySP MIT circular/one-pole idioms,
PCM/stats/excitation/guards/FFT are available; no native rotary exists. Tape's
destructive erase/write medium is not a reusable fractional-delay acoustic buffer.

Original gap: two inertial motor/rotor assemblies with frictional transmission,
and one **localized worn horn-belt section** transported by actual motor motion.
The worn section reduces static/kinetic traction; release and momentum-conserving
recapture depend on torque/relative speed. No random source, reset phase or
arbitrarily perturbed LFO. Prototype outside the repository establishes plausible
slip/catch behavior; independent reference and native tests must prove it later.

## Conservative admission gate

Standalone portable mono host effect, **32–96 kHz**, two 384-float delay histories
(3072 B), 257-point prepared sine table and mechanics/filters. Hard ceiling
**5120 B complete processor / 8192 B ARM object text**; fixed per-sample work,
at most one release/capture decision per assembly, no iterative integration.
Paths from a 1 m receiver distance, horn radius .18 m/drum effective radius .091 m:
maximum3.441 ms (<331 frames at96 kHz), all within owned on-chip-sized buffers.
No SDRAM, external allocator, new memory manager or unproven hardware delay.
Prospective admission ceiling **2000 M7 cycles/sample** (20% at48 kHz/480 MHz;
40% at96 kHz) is a future measurement gate, **not an available/measured budget**.
Host time, source operation count and ARM compilation are separate evidence.

Existing ARM Engine11040 B / bundle11288 / four-bundle Runtime45224 remain fixed.
Main has an optional RAT state, not the shared tagged storage proposed in open
#42. Appending rotary state would enlarge every Engine/pool; adopting a new union
and canonical controls now would overlap the unsettled independent PR. Raw state
may fit the larger RAT capacity, but that is **not current admission/ownership or
callback-performance proof**. Keep this sound-first increment standalone; propose
the smallest registration/admission review after that boundary is settled.

Existing BOOT_NONE128 KiB region, seven actual linked images, first-boot RAT
109784 B span and zero static SDRAM are baseline facts, not free SRAM/CPU. Physical
Seed3 audio/cache/stack/SDRAM are untested. Rerun pinned firmware unchanged and
compile/audit the new standalone object; do not claim rotary firmware execution.
