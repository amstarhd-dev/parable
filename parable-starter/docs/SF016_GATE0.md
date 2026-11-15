# SF-016 Gate 0 — documented original-style four-knob Fender Blender

2026-11-15. Issue62; standalone experimental Soundforge specimen, not production
admission. No Engine, correspondence, patch, presets, firmware, hardware or pilot
changes. PR61/SF015 documentation was read as lessons only; no code, tests or
acceptance criteria from that draft are imported or altered. Libra57 stays parked.

## Starting evidence

Clean `sf-016` worktree from current main
`af159d833bdd0b0e17ccc59fcf7edfa506ed98f5`. No duplicate SF016 PR found. Read project
AGENTS, Constitution, README/status, Soundforge recipe and SF002/003/009/010/014
ADRs/reports, plus SF015 Gate0/report documentation only. Root AGENTS and the frozen
correspondence Lexicon are absent; no replacement invented. Project AGENTS exists.
427 old implementation/header/firmware/fixture files hashed before edits.

Baseline GNU14.2.0, CMake/CTest3.31.6, C++17, Linux x86_64. Fresh Debug38/38,
1560.12s; Release38/38,135.73s, both completed before tracked edits. Commands:

```sh
cmake -S parable-starter -B BUILD -DCMAKE_BUILD_TYPE=Debug
cmake --build BUILD -j2
ctest --test-dir BUILD -j2 --output-on-failure --no-tests=error
# Repeat with Release in a separate, initially absent BUILD.
```

This environment uses the unchanged pinned json3.11.3 source cache via
`-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src`.
That is a dependency cache, not SF015 implementation. Main's old goldens remain
mandatory. Local tests, remote CI, ARM compilation, target execution and founder
listening are separate gates.

## Sources, actual inspections and boundaries

* Fender history and 1974 official owner manual establish the historical four
  controls and two switches. Morrin distinguishes the radically different later
  three-knob device and describes transistor phase splitting/diode recombination.
  These are historical/topology evidence, not measured unit responses.
* Factory-style drawing1015 plus parts list at Schematic Heaven, independently
  viewed scan, and the attributed1995 Experimentalists Anonymous redraw. Fuzz
  Central's Philip Bryant drawing/build list and Pisotones' actual build provide
  independent corroboration of the important component values/control roles.
  No scan authenticates a specific1968 specimen. Use **documented original-style**.
* JMK rev3/2013 has six transistors and changed rectifier/loading; audit only.
  Shields2023 has new circuitry; not substituted. Swell has explicit restrictive
  rights; topology/version discrepancies audited ONLY. No Swell source, model
  parameters, preset, netlist, graphic or audio imported. Our network is separately
  transcribed from the public historical drawings, not its simulation.
* ACME.jl `3832ccafb952ee2ca0a27286c091d613062c23fd`, actual `src/elements.jl:bjt`
  and full MIT LICENSE inspected. Reuse the narrow reciprocal Ebers–Moll current/
  derivative foundation already in main's Muff, not Julia or a general solver.
  Github searches for Blender source found no eligible exact full-circuit DSP.
  Recorded upstream sample collections are not circuit models or granted audio.
* Fairchild3390-93 RevB1997 datasheet: actual2N3391/A NPN identity, hFE250–500
  at2mA/4.5V, CBC2–10pF at10V. Q1/2 are3391A low-noise-selected, Q3–5 are3391
  per historical drawings. Same nominal electrical law; noise not simulated.
* Central1N276 datasheet establishes gold-bonded germanium switching identity,
  reverse leakage/forward/recovery limits, not a unique fitted transfer curve.
  THM/Novender SCEPTRE examples p106 transcribes Cordwell1969's named1N276
  parameters. Original AD0862556 retrieval was403; do not claim original handbook
  read. Equations/physical data are reference facts, not a code-import license.

URLs, exact source hashes and the complete MIT notice accompany the specimen.
The MIT/BSD-2 import allowlist and unspecified global project license do not change.
Reuse main's unchanged PCM, stats/excitation, guard, take03 preparer and owned
Muff passive elimination, TS808 stiff-state integration, FIR/FFT/lab/ARM idioms.

## Resolve source contradictions before implementation

| Item | Evidence / selected interpretation |
|---|---|
| Q1 emitter1.5k vs15k | Factory-style R4 explicitly1.5k; the retrieved1995 redraw actually reads **1k5**, despite another site's description of it. Fuzz Central corroborates1k5. Independent1kHz preamp gain21.66 versus2.54 for15k; collector4.81 versus7.10V. Select1.5k, do not repeat the erroneous description. |
| Sustain bottom680 vs680k | Factory-style R8 list reads680K;1995 redraw/Fuzz Central/Pisotones support680ohm. At1kHz,680ohm gives Q3 input gain1.36→21.59 across Sustain;680k gives18.12→21.95 and makes the internal dry tap almost full preamp. DC alone cannot settle this because C3 blocks DC. Select680ohm and retain both AC/DC experiments. |
| Tone125k/3n vs25k/2.7n | Factory-style list and independent build drawing support25k and2.7nF. Select those, not the redraw's125k label/rounded3nF. |
| Blend/Sustain swapped in redraw |10k pot before Q3 is Sustain;100k between post-Q5 wet and the680ohm preamp tap is Blend. Factory-style RC1/RC2 and build sources corroborate. |
| Tapers | Factory RC1/RC2 audio; independent clone prose disagrees on Blend taper. Expose **electrical divider fractions**, not authenticated mechanical knob rotation. Tone/Level linear electrical fractions. |
| Tone Boost | S2 normal shorts R21=100k between Tone wiper and Level top; boost instead connects Tin through47k to Tone wiper while100k remains. Model that network, no invented high shelf. |
| Bypass | Original SPDT output switch leaves input/preamp loading connected. Preserve it as stopped configuration, not claim a modern true-bypass switch. |

Literal netlist and newly drawn connectivity schematic record every part and
orientation. All ordinary resistors/caps nominal; no tolerance tuning to get an
expected sound. Ideal+9V rail; supply C16=10uF has constant9V and zero AC current.
The sixteen signal capacitor histories remain. Source1k and load1M are declared
Thevenin/amp terminals; no calibrated inductive pickup/cable or ADC/DAC claim.

## Device/dynamic model selected before native DSP

Five reciprocal silicon NPNs: transport IS1e-14A, BF350, BR4, n1, T300K nominal.
BF is inside the source's gain bin but not measured at these sub-mA bias points;
IS/BR are explicit nominal assumptions reused from the established model family,
not a manufacturer2N3391 SPICE card. Constant CBC4pF and CBE20pF per device;
CBC is source-bounded, CBE is a declared storage approximation. Omit Early/high
injection, nonlinear transistor depletion/diffusion, terminal R, noise and heating.
No empirical vintage specimen fit is established by independent solver agreement.

1N276 transcription: I=.56uA*(exp(32V)-1), parallel800k leakage. Report explicitly
that800k is a **shunt**, not series resistance. Source junction capacitance is
`.5p/(.6-min(0,V))^.29 +130ns/V * IS*exp(32V)`.
Use constant zero-bias depletion `.5p/.6^.29` (~.58pF) plus nonlinear diffusion
charge `Q=TT*IS*expm1(32V)`, **TT=(130ns/V)/32=4.0625ns**, not130ns.
Assess half/double explicit capacitances; omitted voltage-dependent sub-pF
variation is a stated approximation. No guessed diode noise/dropout.

SI kT/q; matched nominal26.85C. Offline/stopped5–40C uses declared standard
IS temperature laws (silicon EG1.11/ge EG.67eV,XTI3, fixed beta), not measured
vintage thermal drift. Supported stopped diagnostic supply7.5–10V, BF250–500,
source100–10000ohm, amp100k–2M, capacitance multiplier.5–2. Normal controls0–1.
Zero intrinsic-capacitance SPICE transient aborted and is **rejected research**,
not proof an uncharged/static substitute is adequate. Full nominal model remains.

Independent pinned ngspice44.2, Debian package SHA256
`4d76721dd91416055b1088cfc58626d4db47b91afda7c73258c0968ba6e4d24e`.
Built-in BJT/diode implementations, actual saved branch vectors, loaded full MNA,
Gear2/adaptive integration. No native solver/stamps used to emit expected values.
Initial default shared wrapper's tighter gmin/tolerance setting hit its90s limit;
local wrapper owns explicit gmin1e-12/temp/saves/span checks and bounded deadline.
The shared helper is unchanged. Failed/incomplete runs are not resampled as truth.
Full nodes/current/DC/controls/rail/temp/source/load/attack/pluck/chord references
are generated before native code. Dense-cycle checks supplement whole-history
checkpoints; publish their coverage and floating-point fixture quantization.

## Fault selection from both independent open-diode experiments

Same nominal circuit; independently test healthy, D1-open and D2-open at .005,
.05,.2V inputs, held196Hz,110Hz decay and a chord. Both opens change Q4 DC:
B4~1.16549→1.35555V, C4~2.64712→.75684V. Each removes only its diode's current,
leakage and charge; coupling caps/27k loads remain. At.005V held196Hz, late20–40ms
output RMS healthy.03450V, D1-open.14707V, D2-open.004564V. f/2f fits at that
window give octave/fundamental+11.11/−21.41/−36.49dB. This is short transient
window evidence, not a settled20s or universal THD claim. At harder inputs octave
ratios change; do not force a pure octave or universally fixed fault direction.

Select **D1-open**, common anodeQ4 base→cathode collector-side C5/R13 node. It
alters octaving and loudness over the tested range, without normalizing away a
very quiet low-level D2 response. No second native fault/severity, changed rail,
post fader or arbitrary half-wave shape. Healthy/fault use the same kernel.

## Numerical alternatives and predeclared final gates

A complete fixed nodal Newton solve repeats passive R/C work in ~31 dimensions.
Exact stopped passive elimination reduces it to14 junction-port currents, keeping
all histories and nonlinear loading. Use that specialization, not a generic graph
loader, stale block linearization or speculative transistor cache. A memoryless
whole-unit lookup cannot represent the10uF coupling/bias/feedback recovery;
reject without substituting an invented shape. WDF still needs coupled nonlinear
roots here and no eligible exact Blender implementation was found.

Use BDF2 companions for stiff small junction caps (main TS808's established
stiff-state rationale); exact DC translation removes cancellation without removing
nonlinear memory. Stop-only coefficient preparation and bounded pivoted Newton;
retain true device exponentials, not a new approximation project. Test1x/2x/4x/8x
complete processors before selecting playback. Fine-grid1x is scientific only,
up to3.072MHz for16x/32x reference convergence; not advertised hardware rate.

**Predeclared before native implementation; misses remain failures:**

* DC all loaded nodes≤10uV; terminal currents≤2nA; KCL≤2nA. Device derivative
  finite differences relative1e-4 +1e-10A/V. Keep source/device uncertainty distinct.
* Full-history/dense-cycle independent transient: each node RMS error≤3% of its
  AC RMS +.5mV; jack≤3% +50uV; current RMS≤3% +.1uA.192k scientific nominal
  probes;768k for high-frequency1760/3520/8000Hz evidence. Entire histories count.
*44.1/48/96k, both states, coherent~1760/3520/8k and chord versus high-rate
  converged complete-circuit truth. Full-band relative RMS≤5% (8k≤10%);
  largest nonharmonic false spur≤−45dB/reference RMS. Wanted2f is **not** an alias.
  Report linear filter bandwidth loss separately, retaining the original full-band
  gate rather than replacing it with a filter-matched explanation.
* Zero numerical failures/input/output containment in ordinary reference/listening
  evidence; invalid audio becomes zero with a counter, finite huge input bounded
  and counted. Output finite within±1. Reset is reproducible precharged DC;
  stopped playback0, invalid preparation preserves prior state, bad controls reject.
*≤32 Newton updates/internal sample plus final evaluation, stopped DC≤160;
  residual≤1e-9V runtime/1e-11V stopped. All loops bounded; no audio allocation,
  deallocation, locks, exceptions or IO. Guard tests/ASan/UBSan/ARM graph inspect.
* Complete instance≤32KiB; M7 object text≤24KiB, data/BSS0; measure compiler
  frames/call chains, exponentials and full-processor host cost. No physical
  runtime/stack high-water/cycles/boot claim.2000cycles/base48k is an unmet future
  admission question, not inferred from an ELF/object/host average.

One same-take20s guitar package: restrained/mid/intense Sustain; useful Blend
endpoints/half and Boost alternatives, but no combinatorial hundreds of WAVs.
External DI distinguished from the internal680ohm preamp tap. Raw/common and
individual RMS pairs with all actual PCM/float gains/DC/clips; controls/calibration
identical across each healthy/fault pair. Owned synthetic low/high/pluck/chord
and main RAT/Muff/CMOS/Aquarius comparisons. Deterministic second C++ render.
Exact take03/checksums/full CC-BY4 attribution reused unchanged. Founder listening
approval is a separate unperformed gate; publish download and leave one PR unmerged,
draft if scientific gates miss. No claim about specific artists' recording setups.
