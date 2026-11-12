# SF-007 — two rotors, one worn horn belt

Experimental **reduced electromechanical rotary speaker**, not calibrated Leslie
hardware or founder-approved musical quality. Current-main base **fadcb99**
(approved #44); #42 remains open, untouched. PAR-012 is regression-only.
[Gate0](SF007_GATE0.md) and [ADR-007](adr/ADR-007-rotary-belt-model.md) precede
native code. Frozen Lexicon/root AGENTS.md are absent; project AGENTS and issue#45
define this scope. No affliction activation or other correspondence rule added.

## Listen immediately

Download **parable-rotary-auditions** from the PR's verified CI link (30 days):

- **04_healthy_matched.wav / 05_worn_belt_matched.wav** — primary same-guitar A/B.
  Slow until **1 s**, fast until **5 s**, then slow. Both receive exactly the same
  command and input. 01/02 retain common raw gain1; 00/03 dry raw/matched.
- **06_healthy_horn_matched.wav / 07_worn_horn_matched.wav** isolate the actual
  horn contributions. The fault is easier to inspect here. They have a separate
  -26 dBFS RMS target and disclosed gains; they are not substituted into the mix.
- **08_unaffected_drum_raw.wav** — both versions produced this exact drum signal.
- **09_probe_input.wav / 10_probe_healthy_raw.wav / 11_probe_worn_raw.wav** —
  owned synthetic transient/steady probe, explicitly **not recorded guitar**.
- Open **motion.svg**, then **motion.csv** for exact frames/units and all events.
  Healthy horn green, damaged horn red, its motor gray, independent drum blue;
  traction/required torque show why coupling fails and returns.

Music/taps: **mono PCM16, 44.1 kHz, 581701 frames / 13.1905 s** including 2 s
equal silent tail. Probe: **48 kHz, 384000 frames / 8 s**. No resampling, drive,
noise, cabinet/room convolution or stereo. Source-original WAV, permissions/card,
exact settings, spectra, source/compiler identity and checksums accompany them.
Twelve WAVs plus original; founder decides whether the sound is compelling.

From repository root, C++17 / CMake>=3.15 / Python>=3.9 / curl:

```sh
cmake -S parable-starter -B build-rotary -DCMAKE_BUILD_TYPE=Release
cmake --build build-rotary -j2
ctest --test-dir build-rotary --output-on-failure --no-tests=error
mkdir -p build-rotary/input
curl --fail --show-error --location https://huggingface.co/datasets/Julian-br/GuitarJam/resolve/2d467bfec90af19301b01123494f4b2ba64c5a3a/guitar_jam/test/050.wav --output build-rotary/input/050.wav
echo '22e7b845fc2d71200c72c13fedbeb5f8ac593e48a01f99875f857e0cc5dfecd0  build-rotary/input/050.wav' | sha256sum -c -
./build-rotary/parable_rotary_lab --input build-rotary/input/050.wav --output build-rotary/listen
python3 parable-starter/tools/rotary_reference.py --output build-rotary/reference
cmp parable-starter/tests/fixtures/rotary/mechanics.csv build-rotary/reference/mechanics.csv
```

Pinned **CC0 GuitarJam / Julian-br** clean Strat/Scarlett Solo DI, same source as
SF-006, no license exception for DSP imports. [Permission/provenance](../third_party/rotary/AUDIO_NOTICE.md).
Card is already in `third_party/tape/`; no duplicate committed recording.
Normal DSP/tests/firmware never download it. Custom input requires permission,
mono PCM16/6–24 s; the command transition needs that duration. Default owned
synthetic plucks are labeled synthetic. Every output path/hardlink is checked
against the source before writing, and original bytes remain unchanged.

## Borrow-first decision and actual mechanism

Pinned MIT **cwbudde/algo-dsp 455d4a569936db8e74edbf204476a78258deed78**, exact
`rotary_speaker.go` and LICENSE inspected. Adapt two-band800 Hz, nominal speeds,
directivity and Doppler-history structure. No Go/runtime/heap allocator, stereo/
reflection voice, tanh or speed-ramp LFO imported. That source supplies an
acoustic starting point, **not motor physics**. DaisySP's previously audited MIT
fixed ring/read/one-pole idioms are narrowly adapted; FFT/PCM/statistics/guards
reuse existing owned laboratory code. LeslieSim's tree has no license path,
so no listing/framework imported. Exact pins/hashes/notices and evaluated gaps
are in [provenance](../third_party/rotary/provenance.json).

Two independent assemblies each contain motor and rotor velocity/phase, inertia,
drive torque and bearing load. See ADR for SI values and equations. At stick,
joint acceleration obeys total torque/inertia; belt torque must be within static
capacity. At slip, kinetic friction transmits .7*capacity and opposite torques
accelerate the two inertias separately. Recapture requires relative velocity to
cross zero with sufficient static capacity; momentum-weighted projection loses,
never generates, kinetic energy. **No phase is reset.**

Only one condition changes: a **localized slick/worn section at the horn output
pulley contact**, wear0 -> .96. Capacity is
`.22*(1-.96*max(cos(phi_belt),0)^4)` N m, minimum **.0088 N m**; healthy .22.
Kinetic/static ratio, motor, inertia, load and acoustic settings remain equal.
Motor pulley is assumed nonslipping; its actual motion transports belt material
at omega_motor/4. The damaged contact is at the **output**, so the belt continues
moving while the horn lags. This is not a noisy LFO or a wear oscillator driven
by elapsed time. A good belt section restores capacity and accelerates the horn
into lock. Torque is reflected back onto the motor while slipping. Drum is
separate and unchanged. No scalar output dropout is introduced.

Fixed Euler velocity balance / trapezoidal phase update, one bounded mode decision
per assembly/frame. Start at slow equilibrium, belt phase pi, cleared filters/
histories. Proportional motor control under bearing load leaves actual steady
horn speed **.767 / 6.420 Hz**, rather than exactly .8/6.5 commanded. Bass is
**.564 / 4.892 Hz** versus .6/5. Actual trace, not nominal LFO frequency, drives
the renderer. For this guitar render healthy has0 releases; fault **8 releases /
8 captures**, no containment. The horn hesitates to **~2.574 rad/s (.410 Hz)**
in the slow passage; it does **not fully stall** in this selected experiment.

Torques/inertias/wear geometry are explicit experimental equivalent parameters,
not measured Leslie components. Smith/DAFx Doppler and Modelica clutch references
justify mechanisms, not a particular motor/belt calibration. No belt elasticity,
capstan/tension field, temperature, wear progression, backlash or electrical
motor/controller circuit is simulated. The smooth spatial lobe approximates
contact-wrap averaging, not a measured material distribution.

## Independent proof, convergence and continuity

`rotary_reference.py` uses exact clipped scalar ODE integration for locked horn/
drum and independent finer RK4/event interpolation for the worn horn, with exact
trig and unwrapped phases. No native expected output, table or Euler code.
192 kHz reference refinement against96 kHz: peak **.001979925 rad/s /
5.63488e-5 rad**, within .01/.002 preselected gates. Four native rates check
**19200 velocity and28800 phase comparisons**, plus each release/capture event:

| Native Hz | Peak velocity error rad/s | Velocity RMS | Peak phase rad | Peak event timing s |
| --- | ---: | ---: | ---: | ---: |
| 32000 | .00750765 | .000261429 | .000281522 | .0000520833 |
| 44100 | .00366873 | .000133954 | .000167411 | .0000402849 |
| 48000 | .00380008 | .000137003 | .000157477 | .0000364583 |
| 96000 | .00250291 | .0000998386 | .0000800859 | .0000208333 |

All meet .1 rad/s/.02 rad/2 ms gates. Native96k RMS <.8*native32k RMS;
reference coarse/fine values are numerical uncertainty, not physical accuracy.
Four rates match all14 reference event kinds in the first8 s. Fixture CSVs and
manifest regenerate byte-for-byte. No scientific acceptance gate was loosened. A later CSV unit label was corrected
from dBc to dB-relative-total; the first sanitizer CLI run encountered a stale
binary with the old column name (18/19). The rebuilt complete19-test sanitizer
selection passed; this was metadata/build synchronization, not a DSP discrepancy.

Rework worth retaining: linear trig-table interpolation initially failed the
20% refinement requirement (RMS ratio .821). Replace it with cubic Hermite using
analytic endpoint slopes from the **same 257-point table**. No bigger table/state
or cache. Fixed work, smooth periodic curve and <2e-6 sample geometric-delay
error against exact cos at all four rates. This is separate from audio's linear
fractional interpolation.

Tests prove sample-exact drum motion and audio under the matched commands,
repeated slip/catch, nonnegative capture-energy loss (rounding tolerance1e-12 J),
continuous circular phase increments, bounded delay derivative, zero-input
silence while motors move, invalid-prepare history preservation, reset/stop/rates,
extreme inputs and mix endpoints. A997 Hz probe's maximum raw adjacent sample
delta ~.0122; a DC-source control test separately bounds control-induced steps
after startup. These are explicit stimuli, **not a universal clickless promise**.
Wrapped CSV phases are coordinates, not resets. Trace velocities/angles are
**end-of-sample at(frame+1)/rate**; `pre_*` torques/capacity are evaluated before
the update. Decimation128 frames plus every event, no callback logging.

## Sound, spectrum and honest alias limits

800 Hz complementary split, horn/drum effective radii .18/.091 m, 1 m receiver,
343 m/s. Far-field delay `(1-r*cos(angle))/343` ties pitch to actual angular
motion. Directivity and horn1500–8000 Hz coloration follow angle, not an unrelated
oscillator. Mono two-band balance is source-adapted, not measured polar curves.
No retarded-time radiation solver, reflections, speaker nonlinearity or stereo.

Two384-float histories, **maximum3.441 ms /330.263 native frames at96 kHz**;
linear reads are convex and continuous across integer cells. Known half-sample
frequency-response checks include **-1.60 dB at9 kHz / -11.74 dB at20 kHz**
for48 kHz. HF loss is a real interpolation limitation. The table is cubic; the
**audio interpolation remains linear**, with no oversampling in delivered WAVs.

`alias.csv` explicitly exposes the near-Nyquist problem: separate healthy4/23.8 kHz
probes at48 and96 kHz, equal physical window. At96 kHz, power above the native
24 kHz Nyquist is **2.476e-10 (-96.06 dB relative to total)** for4k, but **.224661 (-6.48 dB relative to total)**
for23.8k as Doppler shifts upward. That energy can fold at48k. This is an
**alias-risk experiment, not measured native alias rejection/noise floor**;
rate-dependent filters/interpolation/mechanics also differ. Full-band input is
not alias-free. No claim that8k coloration eliminates aliasing. `spectrum.csv`
gives actual Hann-windowed synthetic-probe spectra and frame/rate identity.

Same software calibration: DI gain **1.44479717813**, peak .25 normalized; not
guitar voltage. Native output is wet only in auditions, fixed **1/3** sum scaling
for formal bounds, not automatic normalization. Float statistics including tail:

| Music | Raw RMS / peak | Raw DC | Offline matching gain |
| --- | --- | ---: | ---: |
| Dry | .0420824653 / .25 | -4.99290e-5 | 1.68228211 |
| Healthy | .00730292128 / .0560193174 | -4.62795e-6 | **9.69400816** |
| Worn belt | .00702312771 / .0590808466 | -4.58315e-6 | **10.0802066** |

Main matched RMS **.07079457844 (-23 dBFS)**; peaks .543052/.595547. Horn-only
target .05011872336 (-26 dBFS), gains12.5568/12.1959. All raw/final gains/levels/
DC and each raw file's own sample delta are retained. RMS matching is not LUFS
or perceptual approval. Matched full-wave correlation **.865379**, difference RMS
**.0367342**, proving difference persists after level matching, not musical merit.
Drum sample arrays match exactly before writing the shared tap.

## Resource and integration boundary

Complete host/actual M7 object **4392 B**, including3072 B audio histories and
1028 B trig table. No caller buffer, allocation or external SDRAM. <=5120 B
ceiling met. ARM object **3152/0/0 text/data/BSS**, <=8192 B ceiling. Pinned
xPack GCC14.2.1-1.1, original libDaisy/linkers. Audio call graph contains only
Rotary advance/cosine, no library math/alloc/IO/lock/exception helpers; sin/exp/
memset are preparation/reset-only. Static frames process32 / advance80 /
cosine20 / prepare48 B: **not cumulative IRQ high-water**. Four cubic table
evaluations, two mechanical balances and fractional reads per frame; no solver
or catch-up loop. Actual guarded callbacks show no allocation/free/locks/IO.

Representative full-processor host pass for581701 frames **45451 /41816 us**
healthy/fault, excluding allocation/preparation/stats/IO, including command check
and output stores. Under host contention, variable timing, **not M7 WCET**.
2000 M7 cycles/sample remains a **future admission ceiling**, not a passed
hardware benchmark or measured free CPU budget. No board execution occurred.

**No Engine/canonical integration.** Current main has one optional RAT; adding
rotary state would enlarge every Engine/four-bundle pool. Raw rotary state could
fit the larger RAT capacity, but the shared-state change in unmerged#42 is not
approved main. Don't silently adopt/resolve it. Current Engine11040 B / Runtime
45224 B and four saved slots remain unchanged. Smallest next admission step:
review exact additive controls/state sharing after that boundary is settled,
then measure target callback cost, without a new allocator/framework.

Existing firmware recipe is actually rerun separately: seven unchanged linked
images, same source/linker/binary hashes; expected internal full-codec probe
overflow remains **88800 B**. **They do not contain Rotary.** Tape and original
activation/living ARM compile audits remain. Linker regions are not free memory
or physical access/timing evidence. No bootloader/driver/production DSP change.

## Verification, changes and unresolved gates

Clean baseline **28/28 Debug (128.13 s) / Release (74.31 s)**. Full local
**30/30 Debug (202.65 s) / Release (85.82 s)**, unchanged **4/4 TSAN (97.29 s)**.
Final audit-column/registry follow-ups **4/4 Release (4.41 s) / Debug (7.99 s)**.
Final **19/19 ASan/UBSan rerun (164.69 s)** passed with leak checks.
CI outcomes/verified downloads are recorded in the PR after completion; no remote
pass is inferred from local success. Local paths:
`/workspace/work/sf007-{Debug,Release,asan,tsan}`. Use the exact sanitizer flags,
targets and regex in [host CI](../../.github/workflows/host-tests.yml); leaks on,
no suppressions. ARM commands, repo root (substitute writable paths):

```sh
python3 parable-starter/firmware/build.py --cache /workspace/work/par005-fresh-cache --output /workspace/work/sf007-arm
python3 parable-starter/tools/check_rotary_arm.py --compiler /workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --output /workspace/work/sf007-rotary-arm
python3 parable-starter/tools/check_activation_arm.py --living --compiler /workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --output /workspace/work/sf007-activation-arm
python3 parable-starter/tools/check_tape_arm.py --compiler /workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --output /workspace/work/sf007-tape-arm
```

Changes: `parable_rotary.h/.cpp`, private delay reader (model/acoustics); lab,
reference and fixtures (actual audio/plot/independent proof); tests (mechanics,
interpolation/safety/CLI/guards); ARM audit; CMake/workflows (build/check/artifact);
third_party (exact permissions/source/license); registry + specimen-count test,
this report/Gate0/ADR/README/status. All prior DSP/canonical/storage/activation/
living/firmware sources untouched, regression goldens never refreshed. Current-main canonical/living **27 files** and all
**17 deterministic tape receipts/WAVs** match before edits; tape level CSV matches
except intentionally variable host timing. CMOS13/germanium25 files match retained
pre-edit captures whose DSP/runner sources are identical on current main. Seven
actual firmware binaries and29 source/linker hashes match SF006 CI baseline.

Reuse: existing83-line PCM,58-line analysis,31-line excitation,87-line guard used
unchanged; seven-line FFT copied unchanged to avoid moving protected source.
MIT acoustic architecture adapted, primitive ring/one-pole techniques retained;
original gap is the torque/friction/worn-output-belt model and proof. No universal
physics framework, dependency-manager/registry-policy extension or productivity
speedup claim. Count actual code size separately from engineering effort.

Still unproven: physical motor/traction calibration, exact cabinet/microphone/
retarded-time response and perceptual quality, full-band alias quality, native
Engine admission and physical Seed3 boot/CPU/cache/ISR/stack/IO. Human/founder
musical approval remains pending. Next: listen to main and horn A/B with motion
plot, independent review, then the smallest state/CPU admission test if approved.
One unmerged PR; no automatic merge of it or any other experiment.
