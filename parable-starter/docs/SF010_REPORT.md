# SF-010 — TS808 and one open feedback diode

Experimental standalone voice from main `a17f64fe222dc5c9867c693fb3b0e4e69c71f465`.
[Gate 0](SF010_GATE0.md) was committed before native implementation;
[ADR-010](adr/ADR-010-ts808-open-diode.md) records the selected circuit and the
observed numerical revisions. No other PR was merged by this work. #42/#48/#50 were open at the audit.
Main advanced during implementation: #50 was independently merged as ca666a2.
The branch was updated to that main, retaining all merged Muff code/tests/audio/
ARM/sanitizer wiring unchanged. #42/#48 remain untouched. The original baseline
below is explicitly against a17f64f; final combined regression has34 tests.

## Result and admission boundary

The same native C++ circuit creates healthy and faulty WAVs. The fault **removes
D2**, anode=clipping amplifier output/cathode=inverting input, from current AND
Jacobian. D1, R6/P1, C4, op-amps, buffers, bias, controls and gains stay identical.
Finite resistor feedback remains. Independent SPICE predicts larger positive
excursions, physical modeled rail encounters and markedly stronger H2. These are
measured electrical results, not a claim that the fault sounds better or gated.
Founder listening recognition, pleasantness and approval are **pending**.

Selected source is ElectroSmash's documented **TS808 true-bypass/effect-on signal
path reduction**, not a measured factory unit or the original JFET/toggle bypass
network. That reduction is an explicit fidelity limit. Both transistor buffers,
loading, nine external capacitors, selective gain, diodes **in feedback**, active
Tone and Level are retained. No memoryless mid-hump waveshaper/post-output fault.
No Engine, canonical bytes, ROM, activation/preset pool, storage, firmware, living
source, biometric or astrology implementation was changed. No Klon work.

## Source, model and rights

The exact component table, assumptions and drawing pin are in Gate 0 and
[provenance.json](../third_party/ts808/provenance.json). Drawing mirror commit
`ebf6b8604d663c1832d06682e74940830cdc5490`, file
`images/tube-screamer-analysis/tube-screamer-true-bypass-schematic.png`, SHA256
`87399df12eb0c2317238d363893da1eab9db44f16df353f31ddcc34b866c42a9`.
C1=20nF is retained; TS9/clone values were not silently substituted. The author's
original block diagram provides historical context; R.G. Keen independently
corroborates feedback/filter principles, not a second component BOM. Only numerical
facts were transcribed; no diagram artwork/text or unspecified-license code copied.

MIT ACME.jl `3832ccafb952ee2ca0a27286c091d613062c23fd` supplies the reduced
BJT/diode/finite-pole amplifier foundation; full notice retained. GPL BYOD and
TS-808-Ultra were inspected and rejected for code import. Main's code import
allowlist remains MIT/BSD-2-Clause. The separately licensed recording is CC-BY-4.0
data, with original and [attribution](../third_party/ts808/AUDIO_NOTICE.md).

Nominal silicon transport IS=1e-14A/BF=350/BR=4, 300K; diode IS=2.52nA/n=1.752.
Ideal9V/4.5V sources, external1kΩ source/1MΩ load. Op-amp A0=100k, GBW=3MHz,
slew=1V/µs, differential input5MΩ, output75Ω, relative rail±3V. Gain/BW/slew/input
resistance use NJM4558 typical data at±15V; this does **not** calibrate a vintage
JRC4558 or its9V output swing. Output resistance/rail restoring are disclosed
reduced-model assumptions. The continuous internal state law is in ADR-010 and
both independent/native implementations. Intrinsic device capacitances, Early/
high-injection, supply/divider sag, IC offset/bias/noise/common-mode/current limit
and detailed saturated recovery are omitted. No physical-pedal fidelity proof.

## Numerical validation and rejected prototypes

Literal independent ngspice44.2 netlists use builtin NPN/diode devices and a
separate continuous current/capacitor op-amp macro. Native reduced stamps,
Newton routine and outputs are not used to generate references. Fixtures contain
27 transistor points,33 diode points,6 DC cases,63 complex AC rows,14 transient
cases on two sampling grids (7196 rows each),28 harmonic rows and literal netlists.
Manifest pins every netlist/CSV. Actual fresh regeneration and `--check-existing`
passed with **zero numeric difference** on the pinned ngspice44.2 binary.

Strict reltol1e-6 initially aborted near26ns in the fault; rejected. Accepted
Gear2/reltol1e-5/abstol1e-10/vntol1e-7/maxstep.5µs runs are checked for finite
values and full duration. Hard-fault maxstep.125µs refinement changes observed
nodes by at most **0.11445mV**. Harmonics use coherent80–120ms/768kHz resampling;
a coarse200kHz analysis was rejected for underresolving the hard ultrasonic case.

A naive seven-unknown joint solve failed at stiff rail/slew transitions. Exact
piecewise-affine elimination of both amplifier states (at most25 paired2x2
regions per device evaluation) preserves the original continuous equations.
Five nonlinear junction unknowns remain; external capacitors are trapezoidal,
internal stiff amplifier states BDF2. Runtime tolerance1nV, stopped DC1pV.
The tightened DC tolerance eliminates roundoff-driven quiet-state resets.

Device current peak deviation was52.83nA (relative+absolute device gates pass);
DC node peak deviation **0.207µV**. Complex AC peak relative error **2.142%**
against the5% gate over Drive/Tone/Level endpoints/midpoint and100–10000Hz.
Healthy midpoint jack gain at100/1000/10000Hz is1.683/6.655/0.938 V/V: the
R4/C3 selective gain (~720Hz), postclip R8/C5 (~723Hz) and active Tone create
mid emphasis. Level0 is exact signal disconnection, after advancing all states.

All-stage transient error against continuous SPICE (includes stiff internal
amplifier states and hard.8V/10kHz; jack is before normalized /4 and FIR):

| Internal Hz | All-node RMS / peak V | Jack RMS / peak V | Diode peak A |
|---:|---:|---:|---:|
|192000, rejected normal|.014076 / .237145|.001432 / .031797|3.182e-5|
|384000, rejected normal|.007880 / .208469|.000583 / .005865|2.416e-5|
|705600,44.1k×16|.001584 / .083227|.0000458 / .0003905|1.597e-5|
|768000,48k×16|.001160 / .071793|.0000512 / .0004670|1.427e-5|
|1536000,96k×16|.000295 / .044354|.0000159 / .0001865|2.968e-6|

Final normal grids meet10mV RMS/100mV stage peak,1mV jack peak and50µA diode
peak gates, with explicit refinement improvement.4x and8x were rejected for
100mV stage peak, **not** declared successes. Coarse diagnostic1x96k can fail
hard stress; it is not the admitted sound or a physical fault. Normal rate range
is44.1–96k with fixed16x. No alternate exp approximation/block linearization.

At1kHz/.05V/Drive.5, clip H2 changes **36.59µV→212.58mV**; jack H2
**39.88µV→35.10mV**. At.2V/Drive.5, jack H1 changes.18733→.57953V and
H2.11562mV→25.487mV. The exact response varies with amplitude/Drive; no
universal adjective or H2 monotonicity is assumed. H1/H2/H3/H5/H7 and extrema
are compared to SPICE with5%+100µV tolerances. DC remains unchanged.

Coherent96kHz alias probe:4096frames, bin340,7968.75Hz/.15V, Drive.7/Tone1/
Level.5,2s settle. Largest nonharmonic folded bin relative to fundamental:
healthy1x−25.536dBc vs16x−134.805dBc; fault−29.313 vs−82.849dBc
(improvements109.27/53.54dB). This is one reproducible probe, not a universal
alias floor. CSVs retain stage sweeps, all harmonic comparisons and alias data.

## Listening package

CI publishes **parable-ts808-auditions** for30days. Start `04_healthy_matched.wav`
and `05_D2_open_matched.wav`, then compare `01_healthy_raw.wav`/`02_D2_open_raw.wav`.
`00/03` are dry. All six contain the same26s excerpt plus2s silence at48kHz.
20 additional controlled WAVs retain raw and **same-gain** healthy/fault attacks,
long notes, balanced sine and triangle. Together with the exact original and
prepared excerpt: **28 WAVs**,26 native renders. No GuitarJam050/take11 audio in
this package. Old050 remains only an unchanged regression source elsewhere.

New source: Guitar-TECHS v1, Zenodo record14963133, **Player3 performance06**.
Same player/dataset as Muff, **different performance**; record only provides P3
music. CC-BY4.0 credits Hegel Emmanuel Pedroza Villalobos, Termeh Taheri, Wallace
Abreu, Ryan M. Corey, Iran R. Roman. Original dual-mono stereo PCM24/48k32s SHA
`2fd88dc725f475e13686f50ce6c88e7fed3d6b635b62147a69d42e29a5dd6254` is retained.
Source prep checks archive/member hashes and channel identity; takes first26s,
channel0, rounds24→16 nearest/ties-away, no resampling/EQ/dynamics/gain. Prepared
SHA `b1162cf7dfdd5921638e403581541c36c2de73022f3bffd0e111e66302048364`.
A fresh archive preparation reproduced both files byte-for-byte. Full32s was
initially planned, but excerpting leaves2s tail within unchanged shared30s WAV
limit; no alteration of that helper or disappearance of the original.

The same software input gain.406172916 gives.2modelV peak for both; output is
jackV/4. These are software calibrations, not measured pickup/Seed3 voltages.
Whole-file RMS matching targets−23dBFS with.89 peak and16× gain cap (not LUFS):

|Signal|Raw RMS / peak|Offline gain|Matched peak|
|---|---:|---:|---:|
|Dry|.032196 / .200000|2.198867|.439773|
|Healthy|.0175585 / .0578188|4.031930|.233121|
|D2 open|.0268130 / .243927|2.640304|.644040|

Matched RMS is.07079458 for all three. Raw renders use gain1, so attenuation is
visible; no near-silent fault is laundered. Controlled common gains are bounded8×,
identical for both. Every ordinary render requires **zero solver failures,
invalid input, input clamp and output containment**, zero PCM clipping; true-open
D2 current is exactly0. Physical macro rail/slew counters are separate evidence.

`state.csv` retains nodes/caps/op-amp states/currents/residual every128frames at
last internalphase15 time `(frame+.9375)/rate`. WAV decimation uses phase0 and
nominal combined48frame FIR delay; dense80–88ms `polarity.csv`/SVG label physical
and filtered timestamps separately. `stage_stats.csv`, `envelopes.csv`, spectra,
settings and gains preserve actual electrical changes, attacks and output levels.
No claim that graph inspection equals hearing or founder listening approval.

## Resources, bounded work and missed initial budget

Actual pinned Cortex-M7 object: **9096B text/0data/0BSS;8104B complete state**.
Fits initial16KiB text/8KiB state ceilings. Engine/bundle/Runtime/RAT sizes remain
11040/11288/45224/10752B. No production allocation or firmware voice integration.

**The initial computation budget failed:** Gate0 planned4x,24 updates and≤100
candidate evaluations/≤600 exponential calls per base frame. The final accurate
16x/32update/12backtrack implementation has a much looser bound:385 device
calls/internal,6160/base frame,36960 `expm1` calls/base frame healthy (5 per
candidate when D2 open), at most512 five-by-five Newton eliminations and25
paired2x2 amplifier-region cases/candidate. FIR interpolation+decimation≤1538
coefficient multiply/adds/base frame. These are source-derived upper bounds,
not observed workload, measured cycles or a WCET certificate. Typical guitar
candidate counts/internal were~2.65, max5; accuracy took priority over claiming
embedded readiness. This budget miss is an unresolved admission limitation.

Fixed stopped inverse/DC/coefficients; audio has no allocation/free, lock, IO,
JSON, matrix preparation, exceptions or unbounded loop. Actual ARM disassembly
checks all six audio methods plus device current. Whole-object imports are
`cos/sin/pow/expm1/memcpy/memset`; audio imports only `expm1/memcpy/memset`.
Known direct application stack chain56+680+816+904+184=**2640B**, excluding
libm/ISR/high-water; stopped prepare8456+matrices2968=11424B before helpers.
No dynamic/hardware stack measurement. `-O2`, Cortex-M7/hard double FPU, IEEE
finite checks; no fast-math. See audit commands/report/disassembly/`.su` in ARM ZIP.

A separate reset/repeat full-process host pass excludes prepare/trace/IO/analysis
and checks exact audio identity. One GNU14.2 Release run on shared Linux measured
16.325µs/frame healthy /15.975µs fault over1344000frames, before later competing
verification loads; latest run times are in `renders.csv`. No baseline speedup
claim, single-device load isolation or Seed3 deadline inference. **Target cycles,
future2000cycle/sample ceiling, power, boot/audio, physical accuracy and actual
hardware compatibility remain unmeasured.** Successful old firmware links do not
prove this standalone voice's performance or hardware behavior.

## Reproduction and regression

From repository root, GNU14.2/CMake3.31.6/Python3.12.14 locally:

```sh
cmake -S parable-starter -B build-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-Debug -j2
ctest --test-dir build-Debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B build-Release -DCMAKE_BUILD_TYPE=Release
cmake --build build-Release -j2
ctest --test-dir build-Release --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B build-ts808-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -O1" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined -no-pie"
cmake --build build-ts808-asan --target parable_ts808_test parable_ts808_lab -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build-ts808-asan --output-on-failure --no-tests=error -R '^ts808_'
```

Local directories were `/workspace/work/sf010-{Debug,Release,asan}`; PATH prepended
`/workspace/work/baseline-tools/bin` for pinned CMake. Sanitizer reuse of Release's
JSON checkout only avoids downloading that unchanged dependency. No shared-state
change; existing4 activation/living TSAN and19 integration ASan/UBSan cases remain
in CI unchanged, alongside the new2 focused cases. The new test invokes the same
callback allocator/lock/IO guard used by main. It checks44.1/48/96k, endpoint and
midpoint controls, reset, rejected prepare preserving history, NaN/Inf/finite
extremes, sustained notes, fresh silence, tail and bounded counters.

Pre-code baseline30/30 Debug123.62s, Release72.04s. Final Release **32/32,177.56s**;
final Debug/sanitizer and final-head CI results are recorded in the PR when complete.
No old goldens refreshed:131 protected source/firmware/fixture hashes and27
canonical/living evidence files stayed identical. Re-rendered CMOS8/Ge20 WAVs,
tape11 WAV+5 deterministic receipts and rotary12 WAV+6 receipts were byte-identical
to retained main evidence. LabRAT's existing compatibility tests retain its goldens.
Old firmware links/audits and expected non-gating full-JSON/internal overflow are
retained in CI; no FLASH enlargement or parser removal. No TS808 firmware link.

Independent reference regeneration (ngspice44.2 binary on PATH):

```sh
cd parable-starter
python3 tools/ts808_spice_reference.py --output /tmp/ts808-reference \
  --work /tmp/ts808-spice --check-existing tests/fixtures/ts808
curl --fail --location https://zenodo.org/api/records/14963133/files/P3_music.zip/content --output /tmp/P3_music.zip
python3 tools/ts808_input.py --archive /tmp/P3_music.zip --output /tmp/ts808-input
../build-Release/parable_ts808_lab --input /tmp/ts808-input/take06-26s-mono16.wav --output /tmp/ts808-auditions
python3 tools/check_ts808_arm.py --compiler /path/to/pinned/arm-none-eabi-g++ --output /tmp/ts808-arm
```

ngspice Debian44.2+ds-1 amd64 package:
`https://deb.debian.org/debian/pool/main/n/ngspice/ngspice_44.2+ds-1_amd64.deb`,
SHA256`4d76721dd91416055b1088cfc58626d4db47b91afda7c73258c0968ba6e4d24e`.
Extract with `dpkg-deb -x`, add `usr/bin` to PATH; package needs ordinary shared
runtime libraries. Replay temporarily writes large fine-step raw SPICE dumps;
allow≥1GiB scratch. Local replay paths `/workspace/work/sf010-{reference-repeat,spice-repeat}`.
Actual ARM uses xPack14.2.1-1.1/compiler14.2.1 from firmware dependency pins;
libDaisy1289a000bc29dae1bc0ed4129cfa2df40ef71574 remains unchanged.
`python3 firmware/build.py` reproduces the existing seven link configurations;
the new voice has an object/layout/call audit only, as required by this issue.
CI renders the new DI twice and compares every non-timing runner file, includes
exact originals/licenses/references/checksums/compiler/HEAD and merge-source SHA.
CI consumes pinned references; it does not silently regenerate goldens.

## Changed files, reuse and unresolved questions

- New `include/parable_ts808.h`, `src/parable_ts808.cpp`, `src/ts808_device.h`:
  standalone fixed circuit and exact one-open branch, diagnostic access/bounds.
- New `tools/ts808_{lab,stimulus,input,spice_reference}` and `check_ts808_arm.py`:
  same-C++ audio, source prep, independent reference and actual M7 inspection.
- New `tests/test_ts808.cpp`, `test_ts808_cli.py`, `fixtures/ts808`:
  science/safety/audio/reproduction and immutable independent netlists/receipts.
- New `third_party/ts808`: permitted borrow notice, facts/provenance/audio rights.
- Gate0/ADR/report, README/status, registry+its count test, CMake and two existing
  CI workflows: disclose experimental status and run/publish this voice. Old CI
  checks remain; job timeouts increased to accommodate additional work.

Measured reuse, not a fabricated engineering speedup: unchanged PCM83 + shared
analysis58 + excitation31 + callback guard87 = **259 physical source lines**;
unchanged SPICE wrapper20 more. Seven owned FFT lines reused; owned pivot,
companion/FIR/damping patterns adapted to this circuit. Specialized new native
implementation is282+55 lines (header separate), runner150, reference169,
science test221 at measurement. This is substantial new coupled circuit work;
reuse did not remove its nonlinear validation cost. Source research, pre-code
SPICE, rejected solver/4x/8x prototypes and final evidence all occurred2026-11-13
UTC; no invented human-hour or compiler-call speedup percentage. Independent
validation and credible listening evidence, not lines added, are the deliverable.

Unproven: founder recognition/preference; true vintage-unit/JFET loading/device
charge/9V IC accuracy; target CPU/stack high-water/realtime admission; natural
component tolerances, temperature and pickup impedances beyond selected model.
The large hard-bound computation budget is a real limitation, not hidden by
successful object compilation. Next smallest ticket: **founder audition of the
raw and matched pair, then a separate measured target CPU/accuracy admission
experiment if the voice merits it**. No automatic integration or new fault.
