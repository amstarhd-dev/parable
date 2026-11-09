# LabRAT — implementation, listening and validation report

This is one **RAT-derived reduced circuit model**, not a claim of factory-pedal,
LM308 transistor-model or musical fidelity. Read the
[source audit](LABRAT_SOURCE_AUDIT.md) and proposed
[ADR-003](adr/ADR-003-labrat-circuit-model.md). The founder-provided PAR-007 brief
supersedes the earlier plan to integrate the generic PAR-006 processor.

Started on main `bb0635032bfae7b15a2e59de152040bf0a7dcb07` after PR #9. PR #11
was unmerged; only its owned PCM helper, synthetic excitation and FFT lab pieces
were reused. No PAR-006 DSP, patch registration or merger is required by this PR.
Required instructions/constitution/status/architecture, core and tests were read;
none of the required repository reading was missing. Original host Debug/Release
suites passed 4/4 before code changes.

## Listen and reproduce

From repository root with C++17 and CMake >=3.15:

```bash
cmake -S parable-starter -B parable-starter/build/rat-release -DCMAKE_BUILD_TYPE=Release
cmake --build parable-starter/build/rat-release -j2
ctest --test-dir parable-starter/build/rat-release --output-on-failure --no-tests=error
./parable-starter/build/rat-release/parable_rat_lab --output parable-starter/build/rat-auditions
```

The passing Release **Host tests** workflow publishes **parable-labrat-auditions**
for seven days. The PR links the downloadable ZIP. Extract/open the WAVs in a
player or DAW; GitHub Actions has no inline audition player. Source/compiler
identity, checksums, license notices, exact render controls and measurements
accompany them. No WAV binaries are committed.

Default material is **synthetic harmonic-stack plucks/strums**, nine seconds,
48 kHz mono PCM16, 432,000 frames, raw excitation peak 0.15 model-volts. It is not
a guitar recording, physical string/pickup model, amplifier or cabinet simulation.
The renderer invokes the same native `Rat::process` used by the tests and ARM
object. Python is an offline independent SPICE driver, never replacement DSP.

| WAV | Distortion | Filter | Volume | Input multiplier | Circuit delta |
| --- | ---: | ---: | ---: | ---: | --- |
| `00_dry` | — | — | — | 1 | Dry source |
| `01_healthy_low` | .02 | .5 | .7 | 1 | Nominal |
| `02_healthy_medium` | .35 | .5 | .7 | 1 | Nominal |
| `03_healthy_high` | .9 | .5 | .7 | 1 | Nominal |
| `04_filter_bright` | .35 | 0 | .7 | 1 | Nominal |
| `05_filter_dark` | .35 | 1 | .7 | 1 | Nominal |
| `06_weak_input` | .35 | .5 | .7 | .1 | Nominal |
| `07_strong_input` | .35 | .5 | .7 | 2 | Nominal |
| `08_diode_open` | .35 | .5 | .7 | 1 | Positive clipping diode disconnected |
| `09_ruetz` | .35 | .5 | .7 | 1 | 47 Ω / 2.2 µF feedback leg disconnected |
| `10_compensation` | .35 | .5 | .7 | 1 | Feedback capacitor 100 pF -> 100 nF, wrong-part experiment |

Compare healthy low/medium/high, then bright/dark, weak/strong and each variation
against healthy medium. All files are whole-file RMS matched around **−22.5 dBFS**,
slightly below the −22 target to keep shared floating peak <=0.89. Actual gains,
raw/final RMS, peaks and mean are in `renders.csv`; quantized peaks can be half
an LSB higher (<=0.89002). Measurements there precede PCM16 quantization, with no
dither, LUFS or perceptual match claim. Dry is also normalized for listening;
its PCM values are not the unscaled synthetic model excitation.

FIR delay is 48 base samples plus frequency-dependent circuit delay. No alignment
compensation or extra sonic processing is applied. Output duration equals input;
long physical capacitor tails are truncated, so do not judge seamless looping or
file-boundary clicks as a circuit characteristic.

Use a real dry recording later without changing DSP code:

```bash
./parable-starter/build/rat-release/parable_rat_lab --input /path/to/dry-guitar.wav --output parable-starter/build/real-rat-auditions
```

Supported input: little-endian mono integer PCM16 RIFF WAV, one 16-byte fmt chunk,
32,000/44,100/48,000/88,200/96,000 Hz, <=30 seconds, <=6 MiB, <=64 chunks. No stereo,
float/24-bit/extensible WAV or sample-rate conversion. Malformed headers/chunks,
rate/alignment/length/padding/duplicates reject. External recording provenance
cannot be certified by the tool. Include trailing silence and keep the original;
output aliases/hard links to it reject before any output writes.

## Circuit and causal variations

The full nominal component configuration is explicit in `RatCircuit`; normal
Distortion/Filter/Volume controls are separate. Distortion and Filter use the
ported exponential pot-position law; Volume is electrical wiper fraction, not a
claim of measured mechanical audio taper. Configuration validates finite ranges
and prepared-filter stability before committing/resetting. It requires stopped
processing. No live-safe component or knob automation is implemented.

Supported configuration ranges (finite values only; no silent component clamping):

| Parameters | Range / preparation effect |
| --- | --- |
| Sample rate; three controls | 32–96 kHz; each control [0,1], control/Filter pot law or Volume wiper division |
| All resistance fields | 10 Ω–10 MΩ; corresponding input, feedback, clipping, Filter or output admittance |
| Input coupling, two feedback-leg caps, clip coupling, gate/output coupling | 1 nF–1 mF; input/feedback transfer or capacitor companion conductance/history |
| Input RF shunt | 0.1–100 nF; input RC transfer |
| Feedback compensation | 10 pF–200 nF; feedback impedance polynomial (nominal 100 pF) |
| Filter shunt | 1–100 nF; coupled Filter node conductance/history |
| Each diode Is; ideality; thermal voltage | 1 pA–1 µA; 1.5–2.5; 20–40 mV; nonlinear-port inverse table |
| Open-loop gain; GBW; slew | 10,000–1,000,000; 0.1–3 MHz; 0.01–1 V/µs; finite-A coefficients and per-step slew bound |
| Supply; rail margin | 5–12 V; 0.1–2 V, with supply/2−margin >=0.5 V; symmetric model rail bound |
| Diode/feedback connected flags | Explicit discrete branch removal, followed by stopped-state reset |

Those are preparation/numerical guard ranges, **not** validated real-component or
LM308 operating ranges. Numerically unstable prepared coefficients additionally
reject; scientific comparisons cover only the declared nominal/fault fixtures.
Stopped healthy-to-fault/back reconfiguration is tested against a fresh instance;
this proves reset determinism, not click-free/live automation.

The borrowed Rodent/PiRAT feedback polynomial represents 560 Ω/4.7 µF and
47 Ω/2.2 µF branches and the pot/100 pF feedback impedance. Finite open-loop
response extends it; explicit slew/rail limits follow it. Input RC, clipping/
coupling/Filter nodes and passive output/Volume loading are component-derived.
The JFET output buffer is idealized. The 2,049-point nonlinear table inverts the
prepared diode port, not a generic drive curve. Disconnected diodes/feedback
branches are removed from the equations rather than represented by arbitrary bias
or enormous component values. Coefficients/tables are prepared off audio.

| Physical change | Electrical / mathematical change | Measured signal and listening consequence |
| --- | --- | --- |
| Positive diode disconnected | Delete positive exponential current branch; coupling/filter states remain causal | Raw clip node rectifies/charges to about +3.70 V in the sine window. Subsequent AC-coupled waveform is much less diode-clipped; transients and recovery differ. Audition 08. Do not assume a large permanent even-harmonic effect. |
| Ruetz intentional mod | Open the 47 Ω/2.2 µF high-frequency feedback leg; remove its admittance | Relative bass/mid/high gain changes. At Distortion .35 the SPICE preamp gain at 200 Hz drops 31.12 -> 24.54 dB, and near 928 Hz 41.85 -> 24.91 dB. Less input sensitivity/clipping in the tested sine. Audition 09. This is an intentional mod, not random failure. |
| Wrong feedback compensation part | Replace 100 pF with 100 nF; change the feedback impedance and closed-loop response | Increased feedback capacitance reduces upper-band gain/bandwidth and changes clipping/attacks. SPICE AC and nonlinear spectra report the actual change. Audition 10. This is not changing the separate 30 pF LM308 pin capacitor or simulating battery sag. |

The output buffer approximation matters especially with the disconnected diode:
large raw excursions could encounter real JFET headroom/junction limitations that
this ideal buffer omits. Post-buffer faulty-pedal fidelity is therefore **not
validated**. The clipping-node/passive-network comparison below does not establish
those missing transistor behaviors. Likewise, the finite-A/rail/slew approximation
does not prove saturated-feedback recovery of a real LM308.

## Independent reference evidence

Offline **ngspice 44.2**, exact Debian package URL/SHA in provenance.json, produced
333 complex AC points across healthy/Ruetz/compensation circuits and three
Distortion settings. It solves literal R/C/controlled-source networks. Three
independent diode/coupling/Filter/output transient circuits (healthy, diode open,
dark Filter) were simulated with <=0.5 µs steps and recorded after 0.2 seconds;
576 retained time points compare three actual shared-stage voltages.

```bash
python3 parable-starter/tools/rat_spice_reference.py --ngspice /path/to/ngspice --output parable-starter/build/rat-spice --fixtures parable-starter/tests/fixtures/rat
```

The script disables user initialization, supplies an explicit minimal startup and
uses no external proprietary device models. It retains actual netlists/logs/raw
outputs and records hashes. CSV fixtures are committed actual simulator results;
CI checks against them, **it does not rerun ngspice**. The exact pinned simulator
can be downloaded/extracted in a user-owned directory, with no root installation.

Measured gates/results are in `measurements.json`:

- Maximum preamp AC gain error after bilinear frequency mapping: about **9e−7 dB**;
  phase about **6e−6 degrees**. This checks the algebra/port against circuit stamps.
- At the **same physical frequency**, uncorrected bilinear warping contributes up
  to about **0.324 dB** over the 20 Hz–20 kHz grid at 192 kHz internal rate. The
  tiny mapped error is not claimed as real-frequency or physical-pedal accuracy.
- Clipping/tone/wiper waveform error about **0.495 mV RMS**, **0.903 mV peak** across
  the reduced-network fixtures. Tests gate <10 mV RMS / <50 mV peak; exact values
  in the generated report. References share documented idealizations, but use an
  independent simulator/solver rather than the native implementation.
- Gate <0.05 dB mapped gain error / <0.5 degrees phase / <1 dB same-frequency gain
  error. No full vendor LM308 macromodel or measured factory pedal was validated.

Additional actual-kernel measurements:

- Filter response: nine small-signal points across three Filter settings and
  roughly 199 Hz/1 kHz/6 kHz. `frequency-and-level.csv` records fundamental gain.
- Distortion/input response: three Distortion settings, four input amplitudes at
  about 1 kHz, with fundamental, THD+N, second harmonic and mean. For example,
  medium Distortion changes THD+N from about −103 dBc at 0.5 mV input to −11.9 dBc
  at 0.2 V input. This is circuit gain/limiting sensitivity, not output-gain scaling.
- `diode-transfer.csv` records the actual prepared nonlinear-port curves for the
  healthy/open diode. It is a port transfer, not DC transmission through the
  AC-coupled complete pedal. Monotonicity is checked.
- At 1.002 kHz / 0.05 V input, medium controls: healthy THD+N **−12.69 dBc**;
  diode-open **−18.38 dBc**, Ruetz **−19.81 dBc**. Diode-open raw-node DC is about
  **+3.697 V**, output mean **−0.0058** in the declared measurement window.
  Output coupling has not fully settled there; long-run recovery is separately
  tested. The open diode's second harmonic is about −82 dBc in that steady window,
  versus the symmetric model's numerical floor. Numerical symmetry/floor values
  do not measure real-device noise or prove a universal even-harmonic rule.
- At 5.877 kHz / 0.05 V input, high Distortion and bright Filter: complete 1x/4x
  pipeline residual **−14.66 / −50.58 dBc**, improvement **35.92 dB**. Coherent
  8,192-point FFT sums bins outside harmonics below Nyquist, relative to the
  measured fundamental. 1x recomputes circuit coefficients at its own rate, so
  this includes discretization as well as resampler tradeoffs. Not alias elimination
  or a full frequency/parameter/IMD sweep; the single-fixture gate is >6 dB reduction.

## Host and ARM engineering evidence

Existing four tests are unchanged. Debug and Release each now pass **6/6**, adding
`rat_circuit_and_safety` and `rat_lab_measurements`. AddressSanitizer plus
UndefinedBehaviorSanitizer targeted checks pass **2/2**. Tests cover nonfinite/
extreme/bit-pattern input, parameter rejection without changing old state, finite
bounds, exact sample/block/in-place/reset identity, mute/silence/DC recovery,
prepared coefficient stability, model slew bounds and no processing allocations.
Long RC decay is tested over 25-second constant/silent intervals in 1x mode;
4x is exercised with all fault configurations and three rates. Host tests and
simulator references are not ARM execution or musical approval.

```bash
python3 parable-starter/firmware/build.py
```

Pinned xPack GNU Arm Embedded GCC 14.2.1-1.1, Cortex-M7 Thumb/FPv5-D16 hard float,
C++17/O2 and IEEE finite checks produce actual ARM `rat.o`. Final exact size is
recorded in `rat-size.txt` / `report.json`; the measured release object is
**6,452 bytes** text with zero object data/BSS, excluding linked math libraries. The ARM
instance is **10,752 bytes** (host 10,760), including curve/FIR/history/components;
zero object BSS does not mean zero working RAM. Compiler direct frames are
reported in `.su`: process 56, block 24, configure 456 bytes. These are not a
total call-chain/IRQ or high-water estimate.

Estimated arithmetic at 4x: **386 FIR coefficient multiplies/accumulations per
base sample**, four circuit evaluations with **14 IIR coefficient multiplies each**
and roughly twenty additional node/interpolation multiplies each: order **520
multiplies and a similar order of additions/base sample**, plus loop/index/clamp
work. Prepared reciprocals avoid audio-side coefficient division. No exponentials,
iterations, allocation, locks, I/O or throws occur in processing. This static
work count is **not a measured Cortex-M7 cycle/CPU budget**; double arithmetic,
loads/branches, compiler scheduling and cache behavior require device profiling.

All three original firmware images remain unchanged: minimal internal flash
74,008 bytes; fuller SRAM and QSPI images 196,292 bytes each. The full-codec
BOOT_NONE diagnostic retains its 65,380-byte overflow. RAT is cross-compiled to
an object, **not linked into or activated in the audio adapter**, flashed or run.
Upstream firmware startup/nosys/RWX warnings remain documented in PAR-005.
A repeat-output build failure exposed upstream bare `mkdir` under `make -B`;
the recipe now precreates/marks only the build directory old while still forcing
source rebuilds. The corrected command succeeded against the existing output,
without deleting it or changing an upstream linker/SDK.

## Readiness and requirements not yet proven

| Stage | Evidence / limitation |
| --- | --- |
| Source / audit / borrow | Exact revisions/notices; reused analytic feedback/pot law, no unlicensed AudioLib code or generic PAR-006 drive |
| Working circuit-derived native model | Component-causal stages and three modifications implemented and numerically exercised |
| Independent reduced-circuit validation | Real ngspice AC/transient data and declared discrepancy gates |
| Host listening material | Eleven synthetic WAVs generated by shared kernel; human/founder acceptance pending |
| ARM-compiled | Actual object/state measurements, preserved firmware builds |
| Hardware / real guitar / pedal-grade | **Not demonstrated** |
| Specific historical factory revision fidelity | **Not authenticated**: reconstruction plus conflicting sources/blocked primary analysis |
| Full LM308/JFET / extreme fault fidelity | **Not verified**; idealizations above can matter materially |

The last two are substantive outstanding evidence, not marked complete. Supply/
bias/internal 30 pF compensation failure fidelity is **UNSUPPORTED**; only the
selected feedback-compensation deviation has independent circuit evidence. A source
license for the overall Parable project is also still absent; borrowed notices
are retained without assigning the rest of the project a license. No live-safe
component automation, patch v1 changes, processor registration, astrology mapping,
preset activation, storage or hardware controls are added.

Founder judgement: bite, midrange, strong/weak picking response, guitar-volume
cleanup, bass tightness, unwanted artifacts and whether healthy/fault behavior
convinces as a RAT-family instrument. Musical acceptance is not automatic.

Next gate: founder listens/accepts/revises/rejects; obtain a specific agreed factory
schematic or controlled physical RAT traces, then refine/validate LM308 and JFET
headroom/recovery where needed. Permanent patch integration follows that review.
No correspondence mapping is invented in this experiment.
