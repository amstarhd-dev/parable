# SF-009 Gate0 — four-transistor sustain fuzz, before native code

2026-11-12. Current main **a17f64fe222dc5c9867c693fb3b0e4e69c71f465**.
#42 (057312b) and #48 (9f8548c) remain open/unmerged and untouched. No source
import from either branch. Project AGENTS/Constitution/README/status, Soundforge,
ADR001/003/004/005/007, patch/native/activation/storage/living/resource reports
and actual BJT/diode/FIR/PCM/reference/guard code inspected. Root AGENTS and frozen
Lexicon not tracked; issue#49 authorizes this experiment, not astrology activation.

Clean GNU14.2/CMake3.31.6 baseline **30/30 Debug153.77s /Release76.52s**, before code:

```sh
cmake -S parable-starter -B /workspace/work/sf009-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/sf009-Debug -j2
ctest --test-dir /workspace/work/sf009-Debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /workspace/work/sf009-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/sf009-Release -j2
ctest --test-dir /workspace/work/sf009-Release --output-on-failure --no-tests=error -j2
```

130 protected native/firmware/fixture hashes and27 canonical/living outputs
captured before edits. Original RAT/CMOS/germanium/tape/rotary kept intact.

## Source / numerical foundation

Select **ElectroSmash American V3 drawing**, not an invented mixed-edition BOM.
Immutable mirror commit **ebf6b8604d663c1832d06682e74940830cdc5490**, file
`images/big-muff-pi-analysis/big-muff-pi-v3-circuit-stages.png`, Git blob
522df7b5d32239b617c23f9dd17a921eb8b310ea, SHA256
8b01ada743fada0e1e585433b63ab8430c802081fd5458d64682a7833baab053.
Q4 input ->Q3 first clip ->**Q2 second clip** ->tone ->Q1 recovery. Numbering
is explicitly reversed from the issue's common Q3 example. Drawing facts, not
artwork/PCB/text, transcribed into a new numerical network. No image redistributed
or source/diagram license inferred. Kit Rae corroborates topology/numbering,
not a source of substitute component values. The article's text has inconsistencies:
use the drawing's R11=10k and R7=430k, not its prose/BOM omissions or15k assertion.

Borrow MIT ACME.jl3832ccafb952ee2ca0a27286c091d613062c23fd `bjt`'s Ebers–Moll
specialization and analytic exponent derivatives; existing retained Expat notice.
Transport/terminal Is convention must be explicit. Selected reduced silicon
IS=1e-14A, BF300/BR4, thermal voltage at300K; no Early/high-injection/depletion/
diffusion model. These are assumed nominal BC239-equivalent parameters, not
measured commercial-unit calibration. Existing Ge constants/PNP polarity are
not silicon substitutions. Audit-only BYOD1cf22b6… `sim/BigMuff/BigMuffClip.h`
(GPL3) uses fixed gain/bias in that inspected legacy specimen; no code/fit copied.

Reuse owned fixed pivoting/forward-junction damping/companion/FIR patterns from
merged germanium/RAT/CMOS, PCM/stats/excitation/guard and run_spice unchanged.
Gap: full17-node passive coupling/loading with10 nonlinear junction ports (8BJT,
2 antiparallel pairs), exact prepared linear elimination, fixed bounded Newton.
Specialize one network; no generic netlist/solver/framework or shared-DSP refactor.

## Physical scope / failed hypothesis retained

All13 schematic capacitors, four NPNs, both diode-feedback DC-blocking1uF caps,
resistive BC feedback, emitter/base bias, interstage loading and exact linear
Sustain/Tone/Volume endpoints retained. External1k source/1M load explicitly added.
Prepared Schur elimination is algebraic, not unilateral stage isolation. Only
Q2 R15 changes:470k ->**2.2M** (wrong/high resistance/partial-open analogue).

Offline pre-code ngspice44.2: healthy Q2 base/collector/emitter
.692404/4.581404/.060972V. CandidateA R15=1M gives.619835/7.234764/.018808V,
small probe-history effect, not convincing choke. CandidateB2.2M gives
.366908/8.439001/.000002194V: almost cutoff. After a.7V50ms burst, a following
.02V223Hz probe is initially about21% **stronger**, not suppressed, and relaxes
back towards the fresh response. This contradicts the proposed post-overload
suppression; preserve the failed musical hypothesis. No third local component
candidate or imposed gate to force it. Native code must independently reproduce
this actual history, and listening determines whether its weak sustain/gating is
useful; do not claim a proven choking/recovery sound.

Strict trapezoidal SPICE startup failed for the near-cutoff case (timestep too
small at~.1us). Gear2/moderate tolerances complete it; verify full time coverage,
record numerical settings/convergence. A failed reference is not a fault sound.

## Bounds / target admission / new DI

Standalone host-first: fixed4x,193-tap owned FIR, <=24 Newton updates+one final
evaluation/internal sample. Full17-node state reconstructed from exact prepared
matrices, coefficients outside audio, no allocations/free/IO/locks/JSON. Expected
~10KiB complete processor, hard16KiB state /16KiB ARM-object-text ceilings.
Finite input clamp, explicit physical-solver failure/output-containment counters;
ordinary reference/audition must use zero fallback or clipping. Output model volts
scaled by9 for fixed software headroom, not physical Daisy calibration.

Current Engine11040B /bundle11288B /Runtime45224B, optional RAT10752B. No Engine,
patch, pool, storage, living, firmware/pin/linker changes. Even if state fits,
unknown full Newton/M7 cost and pending registrations preclude silent admission.
Future2000cycles/sample goal is unmeasured; estimate static work, compile actual
pinned ARM object/calls/layout, rerun existing links separately, no board claim.

Replace rejected GuitarJam050 with **Guitar-TECHS Player3 music direct input**,
CC-BY4, Zenodo14963133 v1, P3_music.zip MD5
071ba80aecf00f4a31fbd167b3f22198 /SHA256
033489e22600751fb5a1633e7d856b901c6782e0486fa02135e830780d9dbfe2.
Original stereo24bit/48k DI converted deterministically to one monoPCM16 channel,
no EQ/dynamics/resampling; retain exact original, credit/card, checksums and
conversion receipt. Same full new input/calibration to both states; original
synthetic two-attack/probe kept separate. Never alter old goldens/archives.
