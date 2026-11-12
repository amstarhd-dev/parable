# SF-005 — Gemini CD4049 healthy versus bias-leak listening

Experimental audio deliverable; **founder listening approval and physical hardware
validation remain pending**. [Gate 0](SF005_GATE0.md) preceded source edits on main
`8f943681ad933f7d8b65d1d66535aa811c75c48e` (merged #33). Baseline Debug/Release
**23/23**, 116.24 / 73.72 s. Project rules, Constitution, ADR-004, source model,
patch contract and actual prepared Runtime were read. Root AGENTS.md and Parable
Lexicon v1.0 are not tracked; project instructions are `parable-starter/AGENTS.md`.
Issue #41 supplies the authorized fault family; no absent lexicon is invented.

Changed files and reasons:

| Area | Files / purpose |
| --- | --- |
| Physical model | `include/parable_cmos.h`, `src/parable_cmos.cpp`: one leakage branch, loaded DC and correct capacitor initial conditions; MIT device equations untouched. |
| Native contract/playback | `parable_patch.h`, `parable_patch.cpp`, `parable_prepare.cpp`, `parable_core.h`: one additive type, strict quantities and shared fixed nonlinear state; existing storage/activation production sources untouched. |
| Listening/reference/tests | `tools/gemini_lab.cpp`, `gemini_spice_reference.py`, `tests/test_gemini*`, Gemini fixtures, existing activation test's optional mixed-model mode: real Engine audio, numerical/callback/source-safety proof. |
| ARM/CI | Firmware Makefile/adapter/build/report, activation ARM audit, CMake and host workflow: link actual CMOS processing, enforce unchanged pool, publish reproducible audio/evidence and run regressions. |
| Provenance/docs | `third_party/gemini/`, this report/Gate 0/ADR-004, contract/README/status/CMOS/Soundforge registry: exact source permissions, decision/tradeoffs, current evidence and limitations. |

Reuse: original MOS equations and derivatives, 4x FIR, PCM reader/writer,
Soundforge statistics/matching/provenance inventory, simulator wrapper/device
stamps, Codec/prepare/journal/Runtime and actual reference/guarded activation
stress. No generalized solver, plug-in system, memory manager or DSP dependency.
Most new model code is the loaded-DC branch/history initialization; no fresh
waveshaper or replacement transistor fit. Local rework: analytical delay-aware
production verification after the 1x peak miss, and correction of an invalid
registry `generated` label to truthful `not-performed` human audition status.

## Listen immediately

Release CI publishes **parable-gemini-auditions** for 30 days. Download the artifact
linked in the unmerged PR, then compare:

- **04_healthy_matched.wav / 05_bias_leak_matched.wav**: identical clean guitar
  input and settings, whole-file RMS matched. This is the primary A/B.
- **01_healthy_raw.wav / 02_bias_leak_raw.wav**: common output gain=1, preserving
  the actual attenuation from the damaged circuit.
- **00_calibrated_dry.wav / 03_dry_matched.wav**: same calibrated input dry/raw and
  dry matched. `source-original.wav` is the byte-identical downloaded recording.

Six processed/dry mono PCM16 WAVs at **44.1 kHz / 537,601 frames (12.1905 s)**,
plus original. No resampling, cabinet, amp, noise, pitch change or post gate. One
second silence is appended equally to allow the modeled coupling tail to decay.
Optional synthetic/owned input is supported; the CI listening evidence uses a
real DI recording, not synthetic plucks mislabeled as guitar.

From a clean checkout, repository root (CMake >=3.15, C++17, Python >=3.9, curl):

```bash
cmake -S parable-starter -B build-SF005 -DCMAKE_BUILD_TYPE=Release
cmake --build build-SF005 -j2
ctest --test-dir build-SF005 --output-on-failure --no-tests=error
mkdir -p build-SF005/input
curl --fail --show-error --location https://huggingface.co/datasets/Julian-br/GuitarJam/resolve/2d467bfec90af19301b01123494f4b2ba64c5a3a/guitar_jam/test/050.wav --output build-SF005/input/050.wav
echo '22e7b845fc2d71200c72c13fedbeb5f8ac593e48a01f99875f857e0cc5dfecd0  build-SF005/input/050.wav' | sha256sum -c -
./build-SF005/parable_gemini_lab --input build-SF005/input/050.wav --output build-SF005/gemini
```

The source is **GuitarJam / Julian-br**, pinned revision and SHA above, publicly
dedicated CC0-1.0 by its dataset card. Author describes clean Fender Stratocaster
neck/middle -> Focusrite Scarlett Solo -> Audacity, no effects/amp. Card and
[audio provenance/permission review](../third_party/gemini/AUDIO_NOTICE.md) are
retained. This is an external host listening input, not DSP-code import or an
exception to Soundforge's MIT/BSD code-import policy. Normal builds/tests/DSP do
not fetch recordings; only the explicit Release audition step does.

## Actual electrical change

Reuse the SF-002 MIT-adapted complementary MOSFET current/Jacobian and its existing
two-inverter signal-path network. No new device curve, general solver or effect
framework. The [ADR-004 amendment](adr/ADR-004-cmos-inverter-model.md) records the
fixed experiment and physical limitations.

The old `feedback_low` (Rf2 1 Mohm -> 100 kohm) **does not move DC bias**. At DC
the coupling capacitors open, and zero ideal gate current forces gate=output;
the inverter's self-bias root is independent of Rf2. It remains a useful AC control,
not the chosen damaged voice. Independent SPICE confirms this at 6 and 9 V.

The selected malfunction is **gate-2 insulation/bias-path leakage to VSS**, represented
by one resistor **Rb: open -> 220 kohm** (0 -> 4.545454545e-6 S). This is a modeled
conductive damage path, not a measured failed chip or a claimed manufacturer fault
distribution. The unchanged 1 Mohm feedback supplies its DC current:

`g2 = o2 * Gf/(Gf+Gb)`;
`Iinverter(g2,o2) + (o2-g2)/Rf2 = 0`.

| 9 V DC node | Healthy | Bias leak |
| --- | ---: | ---: |
| Gate/output 1 | 3.482808651 V | unchanged |
| Gate 2 | 3.482808651 V | **1.622727171 V** |
| Output 2 | 3.482808651 V | **8.998759769 V** |

The damaged gate is below the model's frozen-parameter NMOS threshold (~1.993 V);
the NMOS is off at rest and larger attacks must drive it into conduction. The
passive coupling/feedback histories make the response state dependent. Capacitors
initialize to the **split** gate/output equilibrium, not an arbitrary bias constant.
The shunt appears in the gate KCL denominator and analytic Jacobian for every
solve; output-node residuals are re-evaluated at the final iterate. No generated
glitches or fallback resets are credited as malfunction audio.

Exploratory Rb values 100k/220k/330k/470k/1M were evaluated in this **same path**.
100k nearly muted the source; 330k shifted bias but gave less cutoff. Only **220k**
is registered as the fixed Gemini malfunction. No severity interpolation, random
failure taxonomy or astrology/retrograde activation rule is introduced.

## Levels and numerical receipts

Both states: Drive=.75, Volume=.5 electrical fraction, fixed stiff 9 V, 4x whole
circuit FIR. One engine amplitude unit=one model input Volt; jack output/4 is the
existing normalized convention. The original DI peak is 0.173034668 digital units;
both receive software gain **0.866878306878** for **0.15 model-V peak**. This is
an explicit software scale, **not physical pickup/codec calibration**.

Representative raw float results, including the same appended tail:

| Render | RMS | Peak | DC | Offline matching gain |
| --- | ---: | ---: | ---: | ---: |
| Dry | 0.026264691 | 0.150000006 | -3.24148e-5 | 0.747629741 |
| Healthy | 0.211781948 | 0.582067966 | 1.17e-11 | **0.0927192534** |
| Bias leak | 0.00140215374 | 0.0635516420 | -4.82e-12 | **14.0043588** |

The damaged raw output is ~**43.58 dB quieter in RMS** than healthy. Do not mistake
normalization for a naturally loud fault. Shared RMS target is **0.0196362641**
(about -34.14 dBFS), limited by the fault's .89 peak. Matched healthy peak .0539689,
fault .88999999; sparse attacks have different crest factors. Whole-file RMS is
not LUFS/perceptual matching. No matching gain enters the native Engine.

Each processed render evaluates **2,150,404 internal phases**, zero failed solves,
maximum observed loops healthy **7**, damaged **4** (budget 16). Maximum observed
last-phase residual is ~1e-10 A; the solver independently checks **every** phase
against its 1e-9 A acceptance limit. Last-phase NMOS cutoff observations: healthy
0, damaged **521,616 / 537,601 base frames**, including the tail. These are sampled
model states, not all-phase measurements or physical transistor observations.

`renders.csv` includes raw/final levels, exact Rb/conductance/DC, calibration,
matching gains, all-phase counters and measured full-Engine host timing. Timing
excludes parsing/preparation/receipt IO and intentionally varies between runs;
the listening samples and deterministic electrical/spectral receipts repeat.

Representative full-Engine timed pass for this 537,601-sample input: healthy
**572,317 us**, fault **475,383 us** on the local x86_64 host, including dispatcher
and Cmos processing but excluding preparation, statistics and receipt IO. Single
observations under host load, not a controlled benchmark or target extrapolation.
`electrical.csv` is decimated **last internal phase** node data, not every phase;
FIR output is delayed 48 base samples. `spectrum.csv` gives coherent frequency/
level/fundamental/THD+N/second-harmonic/DC and selected folded-third 1x/4x figures.
One spectral alias line is not total alias rejection or a listening-quality score.
The runner refuses fallback, numerical-rail clipping and near-silent residue
normalization. Source files/hardlinks are protected against output overwrite.

## Independent validation and disclosed rework

Reuse pinned **ngspice 44.2**, the independent literal MOS equations/passive stamps
and original simulator wrapper. New [generator](../tools/gemini_spice_reference.py)
adds only Rb. Fixtures, generator, netlist hashes/step/alignment and simulator
manifest are committed under `tests/fixtures/gemini/`. No native expected output
or fitted timing shift feeds the independent reference.

| Comparison | Observed | Gate / meaning |
| --- | ---: | --- |
| Six loaded DC cases, 6/9 V | 4.46269e-9 V peak | <1e-5 V; explicit gate/drain KCL checks |
| 96 kHz **1x comparison mode** | 1.98177 mV RMS / **56.716 mV peak** | RMS <5 mV; initial 50 mV peak target **missed**, disclosed; below 1% of rail (90 mV). Not shipped mode. |
| 96 kHz base, actual **4x production mode** | **0.274758 mV RMS / 13.7115 mV peak** | <5/50 mV and >=2x peak improvement; passed |
| Exact **44.1/48 kHz playback**, 4x | **0.510245 mV RMS / 17.5769 mV peak** | <5/50 mV at actual recording / target rates; passed |

Each transient mode checks 3,840 node values across healthy/damaged and .05/.15 V
inputs, not only the final audio sample. Production node alignment is derived
from the **96 internal-sample input-FIR group delay / phase 3**:
`time=(baseSample-24+0.75)/96000`. It is not optimized to minimize error and does
not compensate the real-time signal. The output decimator is not part of these
pre-decimator node observations. Original SF-002 reference tolerances/data remain.

An additional 3,840 independent node values cover **44.1 kHz** (the actual DI
audition) and **48 kHz** (the ARM probe), using `(sample-24+.75)/rate` without
fitted shifts. The 96 kHz test is not being used to stand in for lower playback
rates. Other supported rates have bounded/reset/no-fallback host tests, not a
claim of exhaustive analog-reference accuracy.

This validates the reduced circuit's **numerics**, not a physical CD4049 sample.
Measured MOS fit is only |Vgs|=2.5..6 V; the selected damaged bias uses explicit
endpoint-frozen extrapolation. Supply dropper/sag, protection/body currents,
intrinsic capacitances, temperature/noise/tolerance and actual oxide-damage physics
remain unmodeled. Saturating very hot inputs/solver containment are not fidelity.

## Native integration and ARM evidence

`GeminiCmos` revision 1 uses the existing canonical Codec -> prepare -> Engine.
Required Drive/Volume normalized [0,1], strict `biasLeak` boolean quantity, exact
32–96 kHz rate. No defaults/coercion/severity. Additive registration leaves all old
canonical bytes/semantics unchanged. Old readers reject the unfamiliar type.
One fixed tagged union holds **Rat OR Cmos**, never both. Disabled declarations
also consume that slot; mixed/two-CMOS chains reject. Eight utility stages and
four saved documents/prepared bundles remain. Explicit native control setters
establish union lifetimes; no raw native struct serialization is introduced.

| Resource | Before | After |
| --- | ---: | ---: |
| Host Engine / Runtime | 11,048 / 45,288 B | **unchanged** |
| M7 Engine / typed document | 11,040 / 236 B | **unchanged** |
| M7 bundle / pool / Runtime | 11,288 / 45,152 / 45,224 B | **unchanged** |
| M7 standalone Cmos | 2,128 B | 2,152 B (fits existing RAT union capacity) |
| M7 Cmos object text/data/BSS | 5,604 / 0 / 0 B | 5,548 / 0 / 0 B |
| Activation control/audio objects | 4,949 / 712 B text | 5,181 / 776 B |

Pinned xPack GCC 14.2.1-1.1 / libDaisy `1289a000bc29dae1bc0ed4129cfa2df40ef71574`,
original linker scripts/regions. New **actual ARM ELF**, active damaged Gemini
callback: internal FLASH **101,300 B load span** (<131,072), Berkeley text/data/BSS
**99,604 / 1,696 / 39,784 B**. Ordinary bypass/RAT images now retain CMOS capability
and grow; first-boot LED stays unchanged. Existing full-codec internal probe
overflows by **96,156 B**, its expected reported non-gating layout limitation;
full SRAM/QSPI diagnostics link at 227,076 B. No FLASH enlargement/migration.

Callback direct compiler frame **48 B**, Cmos::circuitSample **344 B**; stopped
Engine::load **2,304 B** uses a small control-stack candidate before commit.
These are not cumulative stack/high-water or deadlines. Actual activation audio
object references only RAT::process, CMOS::process and memset; lock-free uint32
loads/stores/DMB, no helper/exclusive retry, allocation, locking, IO or configuration.
Static work remains 386 FIR MACs/base sample and four <=16-loop 2x2 solves (up to
136 inverter / 272 MOS evaluations), **not measured ARM cycles**. No image flashed.

## Verification, preservation and remaining gates

Final local GNU 14.2.0 / CMake 3.31.6 results: **26/26 Debug (176.79 s)**,
**26/26 Release (90.60 s)**, **18/18 ASan/UBSan (214.47 s)** with leak detection,
**3/3 TSAN (102.49 s)**. ARM build/audit passed. Times include host contention,
not deadline estimates. CI outcomes and artifact links are recorded in the PR.
Tests cover data/round-trip/errors, public model KCL,
independent references, reset/invalid prepare/quiet suppression/rate endpoints,
actual Engine parity/disabled history/edge inputs and four journal slots.
The existing activation stress is reused with healthy/fault/RAT/utility documents:
4,096 publications / four joined restarts, sample-exact reference and callback
allocation/lock/IO guards. Original activation/concurrency tests are retained.

Exact local build/test paths (baseline directories were fresh; then incrementally
rebuilt), from repository root with CMake on PATH:

```bash
cmake -S parable-starter -B /workspace/work/sf005-before-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/sf005-before-Debug -j2
ctest --test-dir /workspace/work/sf005-before-Debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /workspace/work/sf005-before-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/sf005-before-Release -j2
ctest --test-dir /workspace/work/sf005-before-Release --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /workspace/work/sf005-asan -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build /workspace/work/sf005-asan --target parable_core_test parable_audio_safety_test parable_patch_test parable_prepare_test parable_rat_test parable_rat_integration_test parable_patch_audition parable_firmware_measurement_test parable_presets_test parable_activation_test parable_activation_demo parable_cmos_test parable_cmos_lab parable_gemini_test parable_gemini_lab -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir /workspace/work/sf005-asan --output-on-failure --no-tests=error -R '^(firmware_measurement_math_status|core_validation|audio_safety|patch_contract|host_preparation|rat_patch_integration|patch_audio_evidence|rat_circuit_and_safety|preset_journal|activation_contract|activation_concurrency|activation_demo|cmos_circuit_and_safety|cmos_lab_measurements|cmos_lab_cli|gemini_circuit_and_integration|gemini_activation_concurrency|gemini_listening_cli)$'
cmake -S parable-starter -B /workspace/work/sf005-tsan -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-fsanitize=thread -fno-omit-frame-pointer -fno-pie -O1' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=thread -no-pie'
cmake --build /workspace/work/sf005-tsan --target parable_activation_test -j2
TSAN_OPTIONS=halt_on_error=1 ctest --test-dir /workspace/work/sf005-tsan --output-on-failure --no-tests=error -R '^(activation_(contract|concurrency)|gemini_activation_concurrency)$'
python3 parable-starter/tools/gemini_spice_reference.py --ngspice /workspace/work/par007-ngspice/usr/bin/ngspice --output /workspace/work/sf005-spice --fixtures parable-starter/tests/fixtures/gemini
python3 parable-starter/firmware/build.py --cache /workspace/work/par005-fresh-cache --output /workspace/work/sf005-arm
python3 parable-starter/tools/check_activation_arm.py --compiler /workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --output /workspace/work/sf005-activation-arm
```

Substitute writable paths on another host. The existing pinned simulator package
is documented by `third_party/labrat/provenance.json`; ngspice is needed only to
regenerate fixtures, not to run normal CTest. No sanitizer suppression is used.

Legacy RAT 16 and germanium 25 render/report files, CMOS eight WAVs/configuration/
frequency receipts and prior canonical patch audio are checked against pre-edit
hashes. Only CMOS's truthful instance-size field changes in its measurements JSON;
do not falsify it to preserve obsolete metadata. Original LabRAT golden/source
guards are not refreshed. No global project license or correspondence rule changes.

Unproven: founder musical acceptance, actual failed-device characteristics,
calibrated real-guitar electrical IO, physical Seed3 boot/audio/deadline/noise/
power/stack/heap/cache/ISR behavior. In particular a successful ARM link is **not**
a Daisy performance claim. The next smallest step is founder listening of the
matched pair alongside raw levels, then measured Seed3 playback of this same
fixed patch if independently approved. No new architecture or biometric work.
