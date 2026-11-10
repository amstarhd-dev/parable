# SF-003 — positive-ground germanium fuzz specimen

One standalone two-transistor PNP germanium circuit, with healthy configuration
and two causal degraded conditions. This is a **documented laboratory specimen**,
not a calibrated commercial clone or approved musical voice. No astrology/sign,
Engine/patch integration, hardware dependency or previous model rewrite.
[ADR-005](adr/ADR-005-germanium-fuzz.md) records the research gate and refinements;
[provenance](../third_party/germanium/provenance.json) and
[attribution](../third_party/germanium/ATTRIBUTION.md) identify actual reuse.

## Source choice and exact scope

Started on updated main `d628ffc3e0beeca40ca06506c4e1344781be4667`, after PR #17
merged on November 10. Required files were present and read. Clean baseline:
Debug **13/13**, Release **13/13**, full pinned ARM command passed before edits.

Borrowed the MIT ACME.jl `bjt` internal Gummel–Poon current/Jacobian at published
commit `3832ccafb952ee2ca0a27286c091d613062c23fd`. Selected measured AC128 parameters
from Holmes/Holters/van Walstijn DAFx2017, cross-checked against Greg Whitehead's
MIT transcription `7838a40f939eaee39b37afef1dac031ef45c276a`. Both exact notices
retained. No Julia, general solver, framework, recordings or schematic graphics.
The remaining local gap was a fixed native solve for this actual loaded network.

ElectroSmash's inspected classic positive-ground drawing supplies nominal network
values. Its modern Germanium Fuzz and Tayda variants have trimmers/other values;
the paper/Whitehead case study uses 36k/62k/11k/680/22u/100n and an input RC, which
is **not** silently called the same circuit. RT-WDF native Ebers–Moll/Armadillo
implementation was audited: source-header GPL-2.0-or-later OR AGPL-3.0, no import
or adaptation. Root-license absence does not confer rights. Existing MIT/BSD-2
registry import policy stays unchanged; global Parable licensing remains unspecified.

Overton's original link failed; the official Georgia Tech thesis was retrieved.
Measurement sections and model/conclusion assumptions were inspected. Its sampled
transistors had different measured leakage/gain, and its simplified capacitor/
input-cutoff analysis, lookup and manually chosen bias are not adopted. The later
2017 measured study supplies stronger model evidence. Z² DSP's article redirected
to an inaccessible malformed URL; it was not read or passed off as an audit.

## Physical network and parameters

| Element | Selected value / connection |
| --- | --- |
| Battery positive / audio ground | 0 V; negative supply **-9 V**, stiff ideal battery |
| Input | Explicit Thevenin Rs **1 kOhm** nominal, then **2.2 uF** to Q1 base |
| Q1 | PNP; emitter at 0; collector through **33 kOhm** to -9 V |
| Interstage | Q1 collector directly drives Q2 base; **no interstage capacitor** |
| Global feedback | **100 kOhm**, Q2 emitter to Q1 base |
| Fuzz | **1 kOhm** emitter-to-ground pot, wiper bypassed by **20 uF** |
| Q2 collector / output tap | Collector -> **8.2 kOhm** -> tap -> **470 Ohm** -> -9 V |
| Output | Tap -> **10 nF** -> **500 kOhm** Volume divider, **1 MOhm** external load |
| Each transistor BC capacitance | Constant **100 pF**, not a nonlinear depletion model |

Fuzz/Volume are electrical fractions 0..1; endpoints use exact connections, not
tiny invented resistances. A historical mechanical audio taper is not calibrated.
Internal coordinates x=-physical voltage simplify the PNP equations; outputs and
reports convert back to physical signs. This is not a silicon/NPN substitution.
Input and output units are model Volts, with no physical codec level calibration.

Both stages deliberately use the **same published fitted AC128 sample**, not an
authenticated matched pair, universal AC128 or NKT275. Selected internal GP facts:
Is=20.66 uA, betaF=229.6, betaR=14.66, NF=1.133, NR=1.140, thermal voltage25.5 mV,
VAF=19.68 V, VAR=88.28 V, IKF=.463 A, IKR=.2415 A, ISE=2.190 uA,
ISC=7.546 uA, NE=1.796, NC=1.364. GP beta is a model parameter, not an assertion
that a handheld meter will measure that gain at every bias/current.

Includes coupled forward/reverse transport, finite base current, separate BE/BC
low-current leakage, saturation/reverse behavior, Early dependence, high-injection
gain rolloff and the constant BC caps. Omits terminal series resistances,
nonlinear depletion/diffusion charge storage, breakdown, noise, thermal evolution
and battery impedance. The paper's finite sample/measurement range and reported
thermal uncertainty remain. Its Gummel fit ranges include about50..600 mV and
common-emitter measurements +/-5 V; operation at 9 V and altered leakage can
extrapolate beyond those data. No claim to new physical-device matching.

The native API exposes controlled gain/saturation/leakage variations and passive
units within numerical bounds in `validDevice`/`valid`; these are containment
envelopes, not a calibrated device catalog. Reference1x accepts32..192 kHz for
science, normal4x base rates32..96 kHz. Preparation computes DC and coefficients
with audio stopped and rejects failed preparation without modifying the old state.

## Bias, loading and the two degraded conditions

DC bias is solved from transistor currents and feedback. Capacitors are open in
DC; Fuzz/Volume/source resistance do not impose a magic output offset.

| Configuration | Q1 base | Q1 collector = Q2 base | Q2 emitter | Q2 collector |
| --- | ---: | ---: | ---: | ---: |
| Healthy, untrimmed | -0.073218 V | -0.211472 V | -0.161834 V | **-7.552280 V** |
| `q1_leaky` | -0.075817 V | -0.080948 V | -0.060703 V | -8.408716 V |
| `feedback_wrong` | -0.074200 V | -0.134747 V | -0.099255 V | -8.064208 V |

`q1_leaky` increases only Q1's BC leakage saturation current7.546 ->75.46 uA.
It is a controlled degraded-device variation through the original junction law,
not a universal failure threshold or calibrated temperature sweep. Q1 approaches
saturation and Q2 changes bias naturally. `feedback_wrong` changes100k ->10k,
a tenfold wrong part (or equivalent parallel leakage path). It changes input
loading/bias/AC feedback, not an independent waveshaping preset. These are the only
three accepted lab names; unknown names reject. Invalid configurations preserve state.

The measured model predicts untrimmed bias near -7.55 V rather than a popular
-4.5 V target, and input impedance near **1048-j22 Ohm at996 Hz**, lower than
commonly quoted5..8k examples. These discrepancies are visible; no offset/curve is
retuned to hide them. A distinct fitted sample/pair and omitted mechanisms limit
generalization. Impedance is the actual small-signal complex port voltage/current
ratio minus the declared source resistance, measured at1x to avoid a false phase
from uncorrected FIR delay. It includes the coupling cap and feedback.

At0.1 mV/996 Hz/Fuzz.8, source1k ->50k changes output fundamental1.8385 mV
->73.77 uV. At5 mV, the4x THD+N metric changes from-8.51 dBc (1k) to-32.01 dBc
(50k); source loading and level affect cleanup through the same network. This is
an explicit source-R experiment, not all real pickups/cables or a complete guitar
volume-pot/inductor/cable model. Controls, harmonics and frequency response appear
in the artifact CSV; no new sign or correspondence rule is chosen.

## Independent numerical and safety evidence

Pinned ngspice44.2 independently solves physical-sign PNP current-source stamps
of paper equations1,4–7 plus the exact passive circuit, including sourceR and BC
caps. It reads no native expected output. **123 device points**, **7 bias cases**,
**1344 transient rows** (five internal nodes, jack and input current). Transients
include healthy Fuzz0/.5/1, high-source, weak-input and both degraded cases,997 Hz,
up to.05 V, Volume.5; maximum simulator step.25 us over the first12 ms. Native
192 kHz1x matches the standard48 kHz4x circuit integration grid before FIR.

| Gate | Observed discrepancy | Limit |
| --- | ---: | ---: |
| Device currents | 4.79e-9 relative with1 nA denominator floor | abs1e-10 A + relative1e-6 |
| DC bias | 1.684e-8 V peak | 1e-5 V |
| Internal nodes | 1.366 mV RMS /62.90 mV peak | 20 /200 mV |
| Jack | 72.23 uV RMS /1.514 mV peak | 1.5 /10 mV |
| Input current | 0.1343 uA RMS | 1 uA |

At96 kHz the node error is8.701 mV RMS/390.3 mV peak; refining to192 kHz improves
both by over6x without relaxing gates. This is independent solver/discretization
evidence for the **chosen reduced equations**, not new physical pedal measurements.
The reused process wrapper's26.85 C setting does not alter explicit25.5 mV in the
behavioral sources. A research-only builtin GP cross-check gave a different bias;
it is not presented as an exact translation/validation of this model. The selected
reference is the paper's explicit equations, not silently ignored model-card fields.

Final script regeneration reproduces all3 CSVs, manifest and17 netlists byte for
byte. Generated netlists are committed for review; logs remain in local/CI evidence.
Native analytic derivatives are checked against centered differences independently
of the Newton solve. Safety covers all passive/device endpoints, invalid values,
controls/rates, old-state preservation, sample/block/in-place/reset identity,
stopped fault changes, zero Volume, silence/DC/overload recovery, nonfinite inputs,
random float bits, denormals and zero observed processing allocations.

Fresh Debug/Release **16/16**; targeted ASan/UBSan **3/3**. An added paired
underresolved/4x diagnostic is rerun in all three instrumentations. Native check
counts are repetition/invariants, not millions of distinct physical experiments.
The original13 tests are preserved; all sixteen LabRAT WAV/report bytes and ten
protected hashes pass without refreshing fixtures. All thirteen pre-edit CMOS
WAV/report files also match byte-for-byte locally; native RAT/CMOS sources unchanged.

## Numerical containment and listening limits

Fixed four-node solve, stack-only4x4 pivoting, at most24 Newton updates plus one
final evaluation per internal sample. Forward-junction growth damping avoids
inventing a reverse-junction slew limit. Residual acceptance <=1e-10 A. Unsupported
junction candidates (<-12 or>.8 V), nonfinite residuals or iteration failure count
and reset circuit histories to bias, returning zero internally. Nonfinite input
returns zero without state advancement; finite input clamps +/-1 model V, final
output +/-1, tiny values tidy. These are numerical policies, not device faults.

A deliberately coarse1x48 kHz/8800.78125 Hz/.05 V case produces **29 fallbacks in
20480 samples** after exiting the guarded reverse-junction domain. More iterations
did not fix it. It is rejected as listening/alias evidence, not normalized away.
The paired4x case has **zero fallback/clipping**. The accepted.01 V alias fixture
has selected folded-third line -24.0835 dBc at1x versus-113.7972 dBc at4x.
That is one line under stated conditions, **not total alias rejection**, a universal
alias bound or musical approval. Do not use the1x diagnostic as a performance mode.

Twenty9-second48 kHz mono PCM16 WAVs use the same native kernel as tests/ARM:
dry; healthy low/mid/high Fuzz; weak/strong input;50k source;50k plus weak input;
Q1 leakage and wrong feedback. Ten `matched_` files have individual whole-file
RMS gains to common **-24.935288 dBFS**, peak<=.89; ten `relative_` files use
**one common gain1.0**, retaining actual physical model levels. Not LUFS matching.
The leakage case raw RMS is **-67.708 dBFS**, about39.89 dB below healthy mid,
and matched gain is **137.606x**. Raw-relative listening is essential. Ideal noise
is omitted; boosting this quiet simulated fault does not model hardware hiss.

Every processed render has1,728,000 internal solves, zero fallback/output
containment, observed maximum3..8 evaluations. Runner rejects failures/clipping
before normalization; full raw/final levels/DC, negative bias, sourceR, controls,
component changes and gains are recorded. No cabinet/amp/alignment/post-effect.
FIR delay48 base samples plus circuit delay. Output length equals input; tails
can truncate. External owned mono PCM16 uses the unchanged bounded reader; peak
must be<=.5 model V to leave room for the2x comparison. Provenance is unverified
for supplied recordings. No real guitar or human listening performed here.

CI publishes `parable-germanium-auditions` with seven-day retention, source/head
and compiler identity, WAV/source hashes, notices, registry, full configuration,
measurements and exact netlists. A PR's SOURCE_COMMIT may be GitHub's synthetic
test-merge; HEAD_COMMIT identifies the submitted branch. Neither means PR merged.

## ARM resources and feasibility

Actual Cortex-M7/fpv5-d16/hard/O2 object with existing pinned xPack14.2.1-1.1,
libDaisy1289a000… and unchanged SDK/linkers: **7108/0/0 bytes text/data/BSS**,
**2480-byte ARM instance** (2488 host). The object has no allocation/throw/I/O
symbols; unresolved `expm1`, `sqrt`, `sin`, `cos`, `memcpy`, `memset`. Trig prepares
FIR coefficients outside audio; expm1/sqrt are processing dependencies. Object
size excludes final linked math-library cost. Largest own processing frame512
bytes; device104/circuitSample160/process56/block24; preparation2688 bytes.
These compiler frames are not runtime stack high-water or complete library stack.

Static bound per base sample at4x:386 FIR multiply/accumulates; four circuit
solves, <=96 Newton updates and100 full residual/device evaluations, hence
**<=800 expm1 and200 sqrt calls**, plus fixed4x4 work. Healthy-mid audition counts
imply about80.67 expm1/base sample (**3.87 million/s at48 kHz**) and20.17 sqrt.
This is appreciably heavier than the earlier polynomial CMOS model. Actual target
CPU/deadline remains **unmeasured**, and embedded feasibility is unproven. Before
integration, profile the actual M7 math/solve path; investigate circuit-specific
matrix reduction and validated exponential evaluation improvements while preserving
these independent gates. Never substitute a generic clipper to meet a budget.

No germanium processor is linked/activated in firmware. RAT/CMOS objects and all
three existing firmware binaries match the clean baseline exactly: minimal internal
74008 bytes, SRAM/QSPI diagnostics196292 each. The existing full-codec internal
probe still overflows its128 KiB layout by65380 bytes, narrowly expected as before.
Vendor/newlib warnings remain, no new kernel warning. No flashing, boot, physical
audio, target timing/latency/power/noise or hardware procurement claim.

## Exact reproduction and file roles

Run from repository root, CMake>=3.15, C++17, Python>=3.9 for host; local GCC14.2.0,
CMake3.31.6/Python3.12.14. Initial builds used `sf003-before-` directories; final
fresh builds use normal pinned FetchContent, no source override:

```bash
cmake -S parable-starter -B /workspace/work/sf003-final-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/sf003-final-Debug -j2
ctest --test-dir /workspace/work/sf003-final-Debug --output-on-failure --no-tests=error
cmake -S parable-starter -B /workspace/work/sf003-final-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/sf003-final-Release -j2
ctest --test-dir /workspace/work/sf003-final-Release --output-on-failure --no-tests=error
/workspace/work/sf003-final-Release/parable_germanium_lab --output /workspace/work/sf003-final-auditions
python3 parable-starter/tools/soundforge/validate_registry.py
python3 parable-starter/tests/test_soundforge_registry.py
python3 parable-starter/tools/germanium_spice_reference.py --ngspice /workspace/work/par007-ngspice/usr/bin/ngspice --output /workspace/work/sf003-spice-final --fixtures /workspace/work/sf003-fixtures-final
diff -rq parable-starter/tests/fixtures/germanium /workspace/work/sf003-fixtures-final
cmake -S parable-starter -B /workspace/work/sf003-sanitize -DCMAKE_BUILD_TYPE=Debug '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined -no-pie'
cmake --build /workspace/work/sf003-sanitize --target parable_germanium_test parable_germanium_lab -j2
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir /workspace/work/sf003-sanitize -R '^germanium_' --output-on-failure --no-tests=error
python3 parable-starter/firmware/build.py --cache /workspace/work/par005-fresh-cache --output /workspace/work/sf003-after-arm
```

ARM recipe needs Linux x86_64/Python>=3.12 and verifies/downloads missing pinned
dependencies. Optional ngspice uses the existing LabRAT package SHA, not a new
dependency. CI consumes committed independently generated fixtures.

New API/native/device files implement only this specimen; `tools/germanium_*`
provide its renderer/metrics/reference; `tests/test_germanium*` and fixtures gate
safety/CLI/science. CMake/host workflow add tests and audition artifacts; ARM recipe
adds actual object/state evidence. Registry/notices/ADR/report/work log/status
record evidence. Existing shared facilities and RAT/CMOS DSP/tools remain unchanged.
No WAV/PDF/model framework/vendor binary enters Git.

Unproven gates: full physical-unit or manufacturer matching, real guitar, founder
musical acceptance, target feasibility/latency/noise/power/stack, hardware boot and
runtime integration. They remain explicit, independently of lab-specimen review.
Next smallest ticket: founder listens to raw and matched comparisons, then select
an owned physical circuit/pair with measured gain/leakage/rail/source conditions
for calibration, or profile native GP cost before any embedded integration claim.
