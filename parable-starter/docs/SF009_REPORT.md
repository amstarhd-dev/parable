# SF-009 — V3 sustain fuzz / R15 bias-starvation experiment

Standalone experimental sound and circuit study from main a17f64f. [Gate0](SF009_GATE0.md)
was committed before native code. [ADR-009](adr/ADR-009-big-muff-bias.md) records
one documented V3 drawing, exact full coupling reduction and the failed hypothesis.

The intended post-overload choking/suppression is **NOT established**: both
independent SPICE and native DSP show temporary priming/enhancement after hard
attacks for the selected near-cutoff R15 fault. That outcome is retained, with no
imposed gate/dropout or third component candidate. Founder musical judgment,
commercial-unit calibration and Daisy performance remain open gates.

## Circuit, sources and changed files

One source: ElectroSmash **American V3 drawing**, immutable mirror
`mstratman/electrosmash-pdf-backups@ebf6b8604d663c1832d06682e74940830cdc5490`,
`images/big-muff-pi-analysis/big-muff-pi-v3-circuit-stages.png`.
PNG SHA256 `8b01ada743fada0e1e585433b63ab8430c802081fd5458d64682a7833baab053`;
Git blob `522df7b5d32239b617c23f9dd17a921eb8b310ea` independently verified.
[Retrieved analysis](https://electrosmash.mas-effects.com/big-muff-pi-analysis.html)
and [Kit Rae](https://www.kitrae.net/music/big_muff_guts.html) supply context.
Original site returned502; archived HTML hash is an observation, not asserted
identical to the pinned repository's HTML. Drawing facts only: no artwork,
layout/text import or inferred code/diagram license. R11=10k/R7=430k follow
the drawing despite inconsistent prose; no mixing editions.

Selected drawing numbering: **Q4 input → Q3 first clip → Q2 second clip → tone
→ Q1 recovery**. The damaged transistor is its **Q2**, not the issue's common Q3
example. Complete literal independently authored reference netlists preserve
the drawing values,13 external capacitors, resistive base/collector feedback,
both series-capacitor diode-feedback paths and mutual stage/pot loading.
External1kΩ source/1MΩ load and ideal9V supply are explicit assumptions.

`parable_muff.h/.cpp`, `muff_device.h`: one fixed17-node network reduced exactly
on stopped control to10 nonlinear ports, bounded coupled Newton, trapezoidal
capacitor histories and owned193-tap4x FIR. MIT ACME.jl
`3832ccafb952ee2ca0a27286c091d613062c23fd`, `src/elements.jl:bjt`, supplies the
Ebers–Moll specialization/derivative foundation; complete notice retained.
NPN transportIS1e-14A, BF300/BR4,300K and diodeIS2.52nA/n1.752 are **nominal
generic silicon assumptions**, not measured BC239/EHX parameters. Omitted:
Early/high injection, intrinsic junction/diffusion charge, terminal resistances,
noise and supply/temperature dynamics. All13 external charge states remain.
Matching reduced SPICE devices prove that scope. Pinned GPL BYOD legacy code was
audit-only, rejected without import. [Provenance](../third_party/muff/provenance.json).

Runtime coordinates are relative to solved DC, with actual nonlinear junction
voltages/currents evaluated every iteration. This exact translation removed an
initial fresh-zero cancellation floor up to3.74e-8 normalized, without adding a
mute, gate, output filter or block linearization. Final currents are reused
exactly for reconstruction. Prepare/reset are stopped-control operations;
callers must not concurrently edit/read this thread-confined object.

`muff_lab.cpp`/`muff_stimulus.h`: audio/state receipts from that same C++ class.
`muff_input.py`: new-source verification/conversion. `muff_spice_reference.py`,
fixtures/netlists, native/CLI tests: independent physics and safety evidence.
`check_muff_arm.py`: compiled-object/layout/call audit. CMake, ninth registry
specimen, notices, README/status/ADR and existing host/ARM workflows reproduce
the evidence. No Engine, patch bytes/pool, activation, storage, living source,
firmware/linker or correspondence edits. #42/#48 remained open and untouched;
no source imported from them. Missing root AGENTS/frozen Lexicon recorded in Gate0.

## Fault and independent numerical evidence

Only **Q2 R15 collector–base resistance470kΩ →2.2MΩ** changes. This is a
high/wrong-resistance or partial-open analogue, not an authenticated damaged
resistor. Same topology/diodes, Sustain.75/Tone.5/Volume.5 and native gain.

| R15 | Q2 base V | collector V | emitter V | Finding |
|---|---:|---:|---:|---|
| 470k healthy | .692404 | 4.581404 | .060972 | normal bias |
| 1M first candidate | .619835 | 7.234764 | .018808 | probe ratio .997700 at90ms / .999908 at600ms, negligible choke |
| 2.2M selected | .366908 | 8.439001 | .000002194 | near cutoff; following weak probe enhanced |

Two50ms .7V Hann-envelope110Hz attacks at20/220ms, with the same.02V223Hz
probe from80ms, are compared with the probe alone. At90–110ms the selected
fault's preceded/fresh RMS ratio is **~1.23 native48k/4x**, **~1.21 SPICE**,
relaxing towards baseline. Q2 bias, C7 feedback cap and C13 interstage cap
depend on prior attacks: genuine history, not merely constant attenuation.
C7 sign=B2−D2; C13 is across the capacitor beforeR12, not C3−B2.
Seven windows per state/history and long notes are retained. **Suppression failed;
no third candidate, forced envelope/dropout or invented successful choke.**

Pinned ngspice44.2 uses built-in BJT/diodes and the literal schematic, not native
stamps/output:72 device points,29 diode points,6DC cases,27complexAC control
rows,1,380 all-node/cap/current transient rows,28recovery windows and rejected
candidate evidence. Initial strict-trapezoidal near-cutoff startup aborted
(timestep too small); rejected as reference, not presented as sound. Gear2,
reltol1e-6/abstol1e-11/vntol1e-7/gmin1e-12, maxstep.5µs versus2µs complete all
four.65s histories. Full-span/finite checks reject partial runs; uniform5µs
resampling avoids adaptive-step RMS bias. Reference refinement peak difference
**.73275mV /29.56nA** at sampled points.

| Scientific native1x rate | all-node RMS / peak error | jack RMS / peak error | pair-current peak |
|---|---:|---:|---:|
| 96k | 38.17µV /3.172mV | 51.11µV /1.547mV | 230.8nA |
| 192k | 8.04µV /.607mV | 10.34µV /.295mV | 49.14nA |
| 384k | 2.07µV /.100mV | 1.94µV /.0481mV | 9.81nA |

Normal48k/4x integrates at192k plus FIR; scientific jack comparisons are not
unaligned WAV comparisons. DC peak error**1.60µV**; extreme reverse device-current
peak difference**3.558µA (~10ppm relative)**. Modern native SI thermal constants
differ slightly from simulator constants. Recovery RMSrelative**.01294%**,
mean-state peak**42.23µV**. AC maximum complexrelative**3.45%**, including HF
trapezoidal warping. Loaded mid-Tone100/1k/10kHz magnitude**.2897/.1451/.3635**
exhibits the scoop. Sustain0/.5/1 small-signal jack gains**2.316/50.41/208.63**;
source1k bottom safeguard makes Sustain0 nonmute. Volume0 is exact mute.
All endpoint/midpoint tests include loading, not an unloaded article plot.

## Listening package — new real DI and disclosed levels

CI **parable-muff-auditions** (30-day retention) includes21 rendered PCM16 WAVs,
original stereo24 and prepared mono16 input, rights/receipt, all reference
fixtures/netlists, state/spectrum/recovery/settings/levels, source/HEAD/compiler
identities and SHA manifests. Final verified download is linked in the PR.
Start with **04_healthy_matched.wav /05_R15_high_matched.wav**, then raw01/02.

New recording: **Guitar-TECHS v1 Player3 directinput_11**, CC-BY-4.0,
[Zenodo14963133](https://doi.org/10.5281/zenodo.14963133). Credit Pedroza Villalobos,
Taheri, Abreu, Corey and Roman. [AUDIO_NOTICE](../third_party/muff/AUDIO_NOTICE.md)
retains source/license/setup, full attribution, archive/member/prepared SHA pins
and transformations. Authors document a split DI branch separate from amp/mic.
Original21.125s dual-monoPCM24/48k retained; identical channel0 rounds to PCM16,
without trim/resampling/EQ/dynamics. Distinct player/dataset/performance, **not
GuitarJam050**, which is absent from the Muff ZIP. Source pleasantness and
founder preference have not been auditioned/approved by this agent.

Exact same source receives gain**.47641756**, reaching**.2 modelV peak**, plus2s
silence: **1,110,000 frames at48k**. Not physical pickup/Seed3 calibration.
JackVolts/9 is fixed software headroom; rawgain1 for all. Whole-file RMS matching,
not LUFS, targets−23dBFS subject to.89peak and**16x gain ceiling**. Dry crest
limits actual common RMS to**.0353242 (−29.04dBFS)**.

| Signal | raw RMS / peak | offline match gain | matched RMS / peak |
|---|---:|---:|---:|
| dry | .0079380 /.20000 | 4.4500 | .0353242 /.89000 |
| healthy | .0136137 /.040237 | 2.59476 | .0353242 /.104405 |
| highR15 | .00295136 /.019538 | 11.9688 | .0353242 /.233843 |

Fault is**13.28dB lower rawRMS**. The substantial~12x match gain is disclosed,
below16x ceiling; raw evidence remains. All saved normal evidence:**zero solver
fallback, input/output containment and clipped samples**. Internal evaluations
max7 healthy/6fault, mean2.927/2.679. Each render repeats after reset and asserts
identical float output. Two-attack/probe-only1.2s and long-note2s are separately
marked **owned synthetic**, common healthy/fault gain within each stimulus≤8.
`state.csv` is every128frames, last internalphase3 time `(frame+.75)/rate`;
filtered WAV decimates phase0, combined FIR nominaldelay48baseframes. Do not
align physical states directly to emitted WAV. No artificial cap-history clearing
to force the saved2s tail to finish.

## Resources, preservation and reuse

Actual pinned M7 GCC14.2.1-1.1 -O2/hard-float/no-fast-math object:**7008B text,
0data/0BSS;8832B complete processor**,16KiB hard state/text ceilings. Undefined
cos/sin are control preparation; six inspected audio functions call only internal
functions and expm1/memcpy/memset. No allocator/IO/parser/locks/atomics/prepare.
Known application frames process72+sample1072+solve1240+currents256=**2640B**
along that path, **excluding libm/interrupts/high-water**. Control prepare9352+
matrices6232B also needs deliberate stack placement.

Bound4×(24Newtonupdates+finalevaluation)=**100deviceevaluations/baseframe,
1200expm1calls**. Fixed10×10 residual/elimination/backsub is roughly46,000double
multiply/add pairs/frame at that bound, plus Jacobian/reconstruction/history/FIR
work. These are static bounds/estimates, **not cycles/WCET/the future2000cycle
goal**. Full-processor host pass for1,110,000frames: healthy**7.217s/6.50µs per
frame**, fault**6.428s/5.79µs**, shared Linux/GNU14.2; separate reset/process
timing excludes prepare/trace/IO/analysis. Not M7 performance evidence.

Seven existing firmware images actually rebuilt with pinned libDaisy
`1289a000bc29dae1bc0ed4129cfa2df40ef71574`; complete image reports/binary hashes
equal the prior audited build. Load spans: minimal95220B, SRAM/QSPI219720B each,
RAT95356B, LED51592B, bypass109600B, composite109784B. Expected non-gating fullJSON
internal overflow remains88800B. **These firmware links do not contain Muff**;
no bootloader/linker/board compatibility claim.

Engine/bundle/Runtime remain11040/11288/45224B. Muff fits raw10752B optionalRAT
capacity; that alone does not approve sharing/admission. CPU/controlstack/pending
registrations remain gates. All130 protected native/firmware/fixture hashes,
27canonical/living outputs unchanged. CMOS8/Ge20WAV and tape17/rotary19
deterministic evidence files equal prior hashes/verified CI archives; original
RAT16-file golden test passes. No previous goldens refreshed.

Reused unchanged PCM83+analysis58+excitation31+guard87=**259lines**, plus existing
20-line simulator wrapper. Adapted owned pivot/damping/companion/FIR and7-line
FFT techniques; registry/CLI/artifact/ARM conventions reused. New model code:
header61+circuit206+device27=294lines; stimulus20, runner111, reference194,
input49, ARM35, tests149+56lines. These are observed line counts, **not percent
labor saved**. No generic solver/framework. Rework included rejected first
candidate/startup settings, uniform RMS sampling and exact DC stabilization:
reuse did not eliminate the circuit science.

## Reproduce / results

Repository root, C++17/CMake≥3.15/Python≥3.9. Local GNU14.2/CMake3.31.6/Python3.12.14:

```sh
cmake -S parable-starter -B build/sf009-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/sf009-debug -j2
ctest --test-dir build/sf009-debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B build/sf009-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/sf009-release -j2
ctest --test-dir build/sf009-release --output-on-failure --no-tests=error -j2
mkdir -p build/muff-input
curl --fail --show-error --location https://zenodo.org/api/records/14963133/files/P3_music.zip/content --output build/muff-input/P3_music.zip
python3 parable-starter/tools/muff_input.py --archive build/muff-input/P3_music.zip --output build/muff-input
./build/sf009-release/parable_muff_lab --input build/muff-input/guitar-techs-mono16.wav --output build/muff-auditions
./build/sf009-release/parable_muff_lab --input build/muff-input/guitar-techs-mono16.wav --output build/muff-repeat
for file in build/muff-auditions/*.wav; do cmp "$file" "build/muff-repeat/$(basename "$file")"; done
```

Before edits:**30/30 Debug153.77s /Release76.52s**. Final full:**32/32
Debug330.42s /Release100.85s**. New focused Release2/2 31.44s. Full affected
ASan/UBSan **21/21 1004.86s atO0**; focused Muff**2/2 327.63s atO1**. CI preserves
original19sanitizer tests/settings and adds focused2 atO1,15min combinedjob.
No shared-state edits; existing4 activation/living TSAN tests remain inCI.
Focused sanitizer reproduction (same JSON dependency as normal CMake):

```sh
cmake -S parable-starter -B build/muff-asan -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -O1" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined -no-pie"
cmake --build build/muff-asan --target parable_muff_test parable_muff_lab -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/muff-asan --output-on-failure --no-tests=error -R '^muff_'
```

Pinned reference regeneration, Linux x86_64, **not overwriting committed fixtures**:

```sh
curl --fail --show-error --location https://deb.debian.org/debian/pool/main/n/ngspice/ngspice_44.2+ds-1_amd64.deb --output build/ngspice.deb
echo '4d76721dd91416055b1088cfc58626d4db47b91afda7c73258c0968ba6e4d24e  build/ngspice.deb' | sha256sum -c -
dpkg-deb -x build/ngspice.deb build/ngspice
python3 parable-starter/tools/muff_spice_reference.py --ngspice "$PWD/build/ngspice/usr/bin/ngspice" --output build/muff-spice --fixtures build/muff-reference-repeat --check-existing parable-starter/tests/fixtures/muff
python3 parable-starter/firmware/build.py --output build/arm
```

Then run the actual standalone audit:

```sh
python3 parable-starter/tools/check_muff_arm.py --compiler parable-starter/build/arm-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --output build/muff-arm
```
Use the firmware script's `--cache`/`--help` to find the downloaded pinned compiler;
CI uses its explicit cache. A second complete local generation reproduced all fixture numeric values exactly
(max error0), with identical netlists/simulator; no goldens refreshed.
CI checks committed reference/netlist hashes and runs
actual native comparisons; **it does not regenerate SPICE**. Final CI outcomes
and verified artifact SHA/downloads are in the PR. No unexplained failure accepted.

## Open gates / next smallest test

Not tested: measured BC239/EHX matching, actual damaged-component mechanism,
Seed3 boot/audio/voltage calibration, M7 deadline/stack high-water, live editing/
switching/patch admission, subjective pedal recognition or founder listening.
Healthy circuit/control evidence supports a reduced model, not sonic certification.
**Intended post-overload choking remains unfulfilled**; the issue permits this
plainly stated failed musical hypothesis.

Next: founder listen to matched **and raw** new-DI comparisons, explicitly
accept/reject this priming/weak-sustain voice. If actual choking is essential,
scope one separate physical-bias study with new Gate0/independent transient proof.
Do not relabel this negative result or force it with an envelope. Production
admission remains separate and conditional on measured target resources.
