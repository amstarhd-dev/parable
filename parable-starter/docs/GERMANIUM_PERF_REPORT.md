# SF003-PERF — numerical evaluator experiment

Decision: **NO-GO for changing the default**. Preserve the original libm GP
implementation. This is an experimental evaluator/measurement PR, not a claim of
an accelerated shipping processor. Final measurements and CI receipts follow
below. The scope excludes any circuit, component, failure, oversampling, solver,
patch/runtime, firmware activation or correspondence change.

## Baseline and experiment design

Started from main440a99221af7cc3f3b7123db327bb161306d4856 after merged PR19.
Read AGENTS, Constitution, status, ADR005, Soundforge instructions and SF003
report/test/reference records; no required source was missing. Before editing,
fresh Debug and Release each passed16/16, all three complete old audition sets
were captured, and the pinned ARM build passed. Original GP SHA256 remains
89ed1c3fc29d8135bf00ac133ac88b19e32f0f0802183de06add5fd1a84007f0.
The original engine is compiled repeatedly with a private compile-time selector
and isolated test-only namespaces, not copied into alternative circuit engines.
The public API, instance fields,24 Newton updates plus final evaluation, existing
warm start, residual threshold,4x integration and193-tap FIR remain unchanged.

The before-edits benchmark has7 trials of524288 timed frames per processor case;
the paired final comparisons have7 trials of131072 frames per case, interleaving
five implementations with rotating order. Each uses the same deterministic4096
frame buffers,64-frame actual processBlock calls,8192 warm frames and stopped
configuration/reset/allocation outside the timer. Output is consumed after the
timer; separate translation units and no LTO prevent dead-code removal. All6
device current/Jacobian values are consumed in a separate device-only benchmark.
Twelve processor conditions cover healthy/q1_leaky/feedback_wrong, low/nominal
drive,50k source and a100-ohm/.2V/Fuzz1 stress input. Device cases cover nominal,
leaky and supported device-parameter stress, both operating and full-domain
arguments. This sustained coherent workload is not identical to the old pluck
audition's quoted80.67 expm1 calls/base sample.

Local timing: AMD EPYC9V74 KVM VM, CPU0 affinity,3 visible vCPUs, GCC14.2.0,
Linux6.18.57/glibc2.41. Compare actual target-like-O2 and host-O3, both with
DNDEBUG, no fast/finite-only math, no LTO. Frequency is not locked; affinity
does not eliminate VM/load noise. Report observed distributions, not guaranteed
worst execution time or ARM CPU use. Separate instrumented builds count library
calls and approximation evaluations; their timings are **not speed evidence**.

## Mathematical alternatives and scientific gates

[The plan](GERMANIUM_PERF_PLAN.md) fixes all gates before numerical results.
The1pA current floor is100x below the solver's unchanged100pA residual criterion;
relative current/Jacobian terms keep huge mathematical stress values meaningful.
The2uV normal output peak gate is5000x tighter than the existing independent
10mV transient peak gate. Separate raw/node/impedance/spectral and per-sample
event gates prevent a good scalar approximation from hiding changed dynamics.
These are engineering equivalence criteria, not a perceptual threshold.
Offline Decimal90 generates exact hex binary64 constants. Shared signed cubic
Hermite32/64 tables cost2048/4096 coefficient bytes; endpoint-corrected signed
Taylor8/12 polynomials cost72/104 bytes. Neither is per-instance state. Initial
positive-half/inverse-symmetry candidates were slower; signed reduction removes
reciprocal work. Compiler inspection motivated a noinline numerical-device
boundary to reduce solve expansion; the solver itself is untouched. Both initial
negative timing experiments are retained, not presented as the final candidate.

Reduce z by split-ln2 into a binary exponent and a small signed remainder;
evaluate expm1 and the analytic derivative of **that same polynomial**. Chain
each of the four distinct emission scales with constant reciprocal multiplication
inside every device evaluation. Saturation/leakage/gain current products remain
inside every Newton iterate. No voltage rounding, speculative cache, block
linearization, allocation, I/O, lock or table initialization is introduced.

Private supported normalized range[-416,28] covers all four GP domains from the
original ve/vc[-12,.8]V guard. Stable expm1 near zero preserves signed zero. A C1
blend over[-40,-36] reaches the constant(-1,0) asymptote. Derivative absolute
error there is bounded below3.2e-16; relative derivative error in the extreme
tail reaches100% and is explicitly reported. Invalid/private-out-of-domain
arguments return NaN pairs; the original device guard is unchanged. This is not
a physical voltage clamp. Large positive mathematical stress can have large
absolute exp errors; scaled errors and actual current/Jacobian gates matter.

The accuracy runner sweeps full-domain/random/zero/tail/range/cell boundaries,
all6 device current/Jacobian outputs across32 gain/leakage endpoint combinations
plus nominal/leaky, and observed physical Q1/Q2 states. Circuit comparisons use
raw volts/amperes, including attack, DC step, overload/recovery, passive/device/
rate/control/rail/source endpoints, all faults, underresolved1x diagnostics and
coherent response/harmonics/THD+N/1x input impedance. Failure/containment events
are compared per base sample, not merely averaged. Tests retain every original
independent ngspice threshold:123 device points,7 bias cases,1344 transient rows.
Original SPICE fixtures, netlists and generator remain unchanged.

No external exponential code was imported; these are standard local interpolation/
Taylor constructions. Existing ACME/Whitehead MIT notices remain applicable to
the narrowly duplicated device equations. This does **not** establish a public
physical BJT primitive, universal part accuracy or measured reuse by another
circuit. Rangemaster/Tone Bender reuse remains prospective.

## Reproduction

From repository root (CMake>=3.15,C++17,Python>=3.9; pinned ARM recipe requires
Linux x86_64/Python>=3.12):

```bash
cmake -S parable-starter -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug -j2
ctest --test-dir build/debug --output-on-failure --no-tests=error
cmake -S parable-starter -B build/release -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/release -j2
ctest --test-dir build/release --output-on-failure --no-tests=error
python3 parable-starter/tools/germanium_perf/run.py --build build/release --output build/perf-O3 --trials 7 --repeats 32
cmake -S parable-starter -B build/O2 -DCMAKE_BUILD_TYPE=Release '-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG' -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/O2 --target parable_germanium_bench -j2
python3 parable-starter/tools/germanium_perf/run.py --build build/O2 --output build/perf-O2 --trials 7 --repeats 32
cmake -S parable-starter -B build/count -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DPARABLE_GERMANIUM_COUNT_BENCH=ON
cmake --build build/count --target parable_germanium_bench -j2
python3 parable-starter/tools/germanium_perf/run.py --build build/count --output build/counts --trials 3 --repeats 1
./build/release/tools/germanium_perf/parable_germanium_accuracy hermite64 polynomial12 > build/accuracy.json
python3 parable-starter/tools/germanium_perf/baseline.py --output build/original --trials 7 --repeats 128
python3 parable-starter/firmware/build.py
python3 parable-starter/tools/germanium_perf/arm.py --cache parable-starter/build/arm-cache --output parable-starter/build/arm/germanium-perf
```

Evidence under tests/fixtures/germanium-perf preserves initial observations and
the original driver, final uninstrumented trials/metadata and separate counters.
CI republishes paired benchmarks for both optimization levels, original/reference
WAVs and separate experimental H64/P12 matched/common-gain-relative sets with
head/checkout/compiler/source identity. Artifacts expire after7 days; regenerate
them with these commands. No listening/founder approval follows from numerical
agreement or PCM equality. ARM compiled is not ARM measured.

## Final host performance (measured, not inferred)

All times below are ns/base sample. Speedup=libm time/candidate time; below1 is a regression. Per-trial/best/median/worst for every condition are committed in the O2/O3 JSON. Initial clean-main trials are retained separately; compare the paired reference to each candidate, not absolute timings taken at different VM times.

| Evaluator | Fixed coefficient bytes | O2 device operating median ns | O2 healthy nominal full median ns | Full speedup | ARM text incl rodata | Mathematical result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| libm | 0 | 45.25 | 3884.5 | 1.000x | 7108 | reference |
| hermite32 | 2048 | 51.24 | 4466.9 | 0.870x | 9812 | fails fixed gates |
| hermite64 | 4096 | 52.47 | 4543.5 | 0.855x | 11860 | passes tested gates |
| polynomial8 | 72 | 50.90 | 4285.0 | 0.907x | 7664 | fails fixed gates |
| polynomial12 | 104 | 62.89 | 4703.0 | 0.826x | 7696 | passes tested gates |

H64 O3 device-only operating improvement is1.061x (45.36→42.76ns), full-domain stress1.385x (39.69→28.66ns). At O2 the operating device regresses45.25→52.47ns while full-domain stress improves38.94→32.66ns. This domain-dependent microbenchmark cannot substitute for full-processor measurements.

| Processor case | O3 libm median | O3 H64 median | O2 libm median | O2 H64 median | O2 speedup |
| --- | ---: | ---: | ---: | ---: | ---: |
| healthy low .005V/1k | 3005.8 | 2969.2 | 3277.3 | 3482.9 | 0.941x |
| healthy nominal .05V/1k | 3633.7 | 3661.2 | 3884.5 | 4543.5 | 0.855x |
| healthy source .05V/50k | 2625.6 | 2679.6 | 2881.1 | 3187.4 | 0.904x |
| healthy stress .2V/100R Fuzz1 | 3933.6 | 4258.9 | 4266.4 | 4926.7 | 0.866x |
| q1_leaky low .005V/1k | 1973.8 | 2014.1 | 2105.6 | 2311.2 | 0.911x |
| q1_leaky nominal .05V/1k | 2459.5 | 2524.7 | 2611.7 | 2887.7 | 0.904x |
| q1_leaky source .05V/50k | 1611.5 | 1613.2 | 1709.3 | 1912.2 | 0.894x |
| q1_leaky stress .2V/100R Fuzz1 | 3853.2 | 4042.8 | 4101.1 | 4728.0 | 0.867x |
| feedback_wrong low .005V/1k | 2582.4 | 2574.2 | 2801.6 | 3148.2 | 0.890x |
| feedback_wrong nominal .05V/1k | 3309.8 | 3335.2 | 3577.0 | 4173.3 | 0.857x |
| feedback_wrong source .05V/50k | 2583.9 | 2512.9 | 2769.2 | 2962.8 | 0.935x |
| feedback_wrong stress .2V/100R Fuzz1 | 4119.6 | 4171.4 | 4201.8 | 5066.9 | 0.829x |

For healthy nominal O2, observed best/median/worst:
- libm: 3768.8/3884.5/4129.4ns.
- hermite64: 4376.3/4543.5/5349.0ns.
- polynomial12: 4532.0/4703.0/5109.6ns.

Across all12 O2 cases, H64 median CPU time grows6.3..20.6%; P12 grows16.7..26.4%. O3 H64 speedup ranges0.924..1.028x with mixed small outcomes; P12 is slower throughout. No consistent target-like full-processor benefit is demonstrated. ARM cycle benefit is unknown.

### Measured operation counts and bounded work

Separate wrapped/instrumented builds measure actual library calls and approximation calls. For healthy nominal, one4096-frame window after warm-up:
- libm: 514192 expm1, 0 approximation, 64274 solver-loop evaluations; sqrt **library** calls=0.
- hermite64: 0 expm1, 514192 approximation, 64274 solver-loop evaluations; sqrt **library** calls=0.
- polynomial12: 0 expm1, 514192 approximation, 64274 solver-loop evaluations; sqrt **library** calls=0.

Nominal libm125.53515625 expm1/base sample projects to6,025,687.5 calls/s **if driven at48kHz**; optimized candidates replace exactly that count with approximation evaluations. This calls/s value is workload-count×sample-rate, not measured real-time processing/ARM capacity. Default remains libm, so default call counts are unchanged. Other benchmark conditions span64..141.697265625 expm1/base sample (3,072,000..6,801,468.75 at48kHz). All12 benchmark conditions have zero fallback/containment and matching solver work. Counting timings include instrumentation overhead and are not used above.

In the86-case raw/spectral matrix, worst observed base-sample solver work is33 loop evaluations; individual internal solves remain bounded by24 updates plus final evaluation. Static4x bound is100 loop evaluations/base sample,200 device evaluations,800 exponentials/approximations (preparation excluded). Actual longest measured wall-time trial is an observation, not a static worst-case bound. The one underresolved1x stress fallback occurs at the same base sample in reference/H64/P12; no new normal fallback or containment is accepted.

## Accuracy results

Function column is max(abs(error)/max(1,abs(expm1(z)))); derivative column is maximum fraction of the fixed(5e-16+5e-9*exp(z)) gate. Current/Jacobian columns are fractions of their fixed A/S gates. Values<=1 in gate-fraction columns pass.

| Candidate | Scaled function error | Derivative gate fraction | Current gate fraction | Jacobian gate fraction | Full matrix |
| --- | ---: | ---: | ---: | ---: | --- |
| hermite32 | 7.12759e-11 | 2.04313 | 0.786099 | 1.02233 | not selected; math fails, matrix skipped |
| hermite64 | 4.46654e-12 | 0.255032 | 0.0472446 | 0.127544 | 86 cases pass |
| polynomial8 | 1.76492e-11 | 0.044344 | 0.39305 | 0.0221714 | not selected; math fails, matrix skipped |
| polynomial12 | 2.2204e-16 | 0.0340761 | 5.20359e-06 | 4.10284e-07 | 86 cases pass |

Each candidate checks612676 scalar points, plus all6 device outputs in34 parameter sets×8264 voltage pairs and physical state samples. Signed zero, invalid inputs, tails and boundary jumps are explicit tests. H64 maximum absolute function/derivative error in huge-positive mathematical stress is3.19775/1827.18, respectively; P12 both0.000244141. Do not confuse those absolute errors at exponential magnitudes near1e12 with operating-current errors. Tail derivative relative error reaches1, bounded absolutely as explained above. H64 boundary value/derivative gates use at most.03668/.00371 of their limits.

H64 across86 cases: raw output peak7.451e-9V; nodes peak1.117e-10V; input-current peak3.864e-16A; DC bias peak1.631e-11V. P12: output peak1.819e-12V, nodes2.193e-12V, input current1.207e-17A. Configuration acceptance, per-base fallback/containment and solver-work counts match exactly. All86 configurations were accepted; rejected candidate matrix entries are never treated as zero-error evidence. RMS, spectral, impedance and per-case work/error results remain in accuracy.json.

Original independent123-device/7-bias/1344-transient cases pass unchanged thresholds for both H64/P12; direct optimized-current checks cover every device fixture. Four **additional independently solved** SPICE cases cover6V rail/100R source, leaky6V/100k source, wrong-feedback/100k source and40/300 gain endpoints on6V. All five evaluators pass their4 bias/768 transient rows under the original thresholds: H64 node RMS1.354mV/peak45.323mV, jack RMS71.775uV/peak1.097mV, input RMS.554uA, bias peak27.879nV, zero fallback/containment. This reference error is dominated by integration/reference discretization, not evaluator differences. Committed netlists/manifest and spice_corners.py reproduce it with the existing pinned ngspice44.2. These are independent solutions of the same **reduced** circuit, not physical-unit calibration or manufacturer fidelity.

## ARM resource and compiler evidence

Actual pinned GCC14.2.1 Cortex-M7 hard-float/O2 objects; not linked/executed as germanium firmware. Default object is byte-identical to the unchanged-main capture.

| Evaluator | Text incl rodata | Shared coefficients | Data/BSS | Instance | expm1 relocation sites |
| --- | ---: | ---: | --- | ---: | ---: |
| libm | 7108 | 0 | 0/0 | 2480 | 4 |
| hermite32 | 9812 | 2048 | 0/0 | 2480 | 0 |
| hermite64 | 11860 | 4096 | 0/0 | 2480 | 0 |
| polynomial8 | 7664 | 72 | 0/0 | 2480 | 0 |
| polynomial12 | 7696 | 104 | 0/0 | 2480 | 0 |

Actual section detail, exported/unresolved symbols, ELF hard-float attributes, disassembly, stack-usage files and command lines are emitted by arm.py and retained in ARM CI evidence. Shared constants occupy readonly sections, never per-instance RAM. There are no allocation/exception/I/O symbols in any numerical audio object. All retain1 vsqrt.f64 site and1 exceptional sqrt relocation. Baseline valid roots already use hardware sqrt: no square-root approximation or proportional libm-call speedup claim is justified.

Static object instruction sites change vdiv.f64 51→43, vmul.f64 43→62, vfma.f64 15→25 for H64. These include all object functions, **not dynamic instruction/cycle counts**. H64 exp helper is648 bytes, device648; reference device656. P8/P12 helper476/device648. P12 has the same static loop instruction sites as P8 but more polynomial iterations; do not infer equal runtime from those counts. The aggregate main.text section is6084 bytes for all variants; the solve symbol is1272 bytes in each. This is compiler output evidence, not a rewritten solver.

Compiler static frame records: solve512B, circuitSample160B, process56B, processBlock24B unchanged. Reference device104B; candidate device192B plus H64 helper64B or P12 helper48B. This adds a nested static frame cost, not a measured stack watermark or complete bound including library/interrupt callees. No target M7 cycles, cache behavior, memory bandwidth or physical-stack measurement is available. The existing diagnostic full-codec internal-FLASH probe still intentionally overflows by65380B; SRAM/QSPI/minimal firmware builds remain unchanged and no memory region was enlarged.

## Final verification, changed files and remaining criteria

Fresh local Debug **21/21** (160.49s), Release **21/21** (32.50s), targeted
ASan/UBSan **8/8** (243.55s), including original safety/CLI/independent cases,
new fixed-gate accuracy and additional ngspice corners. Table-generator --check,
registry validation and git diff --check pass. Original libm/RAT/CMOS renders
were regenerated: all25 germanium,16 LabRAT and13 CMOS original WAV/report files
are byte-identical to the clean-main captures. Existing protected LabRAT hash
test remains enabled. Both candidate renderers also generate20 actual WAVs each,
with raw-level metadata, matched and common-gain-relative versions; no normal
render fallback/containment. These are the existing same C++ renderer and shared
utilities, not a second DSP renderer.

Exact additional local sanitizer commands:

```bash
cmake -S parable-starter -B build/sanitize -DCMAKE_BUILD_TYPE=Debug '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined -no-pie'
cmake --build build/sanitize --target parable_germanium_test parable_germanium_lab parable_germanium_accuracy parable_germanium_corners parable_germanium_lab_h64 parable_germanium_lab_p12 -j2
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/sanitize -R '^germanium_' --output-on-failure --no-tests=error
./build/release/parable_rat_lab --output build/rat-auditions
./build/release/parable_cmos_lab --output build/cmos-auditions
./build/release/parable_germanium_lab --output build/germanium-auditions
./build/release/tools/germanium_perf/parable_germanium_lab_h64 --output build/hermite64-auditions
./build/release/tools/germanium_perf/parable_germanium_lab_p12 --output build/polynomial12-auditions
python3 parable-starter/tools/germanium_perf/preservation.py --rat build/rat-auditions --cmos build/cmos-auditions --germanium build/germanium-auditions
python3 parable-starter/tools/germanium_perf/spice_corners.py --ngspice /path/to/pinned/ngspice --output build/spice-corners --fixtures build/spice-corner-fixtures
diff -rq parable-starter/tests/fixtures/germanium-perf/spice-corners build/spice-corner-fixtures
```

Local directories are /workspace/work/sf003perf-final-Debug,
sf003perf-final-Release, sf003perf-final-O2, sf003perf-sanitize and
sf003perf-after-arm; the pinned cache is /workspace/work/par005-fresh-cache.
Commands above use portable build/ equivalents. Local baseline drivers and
evidence retain exact source/compiler/flag identities; no unavailable hardware
is implied by path names or successful compilation.

File roles: three evaluator-selector edits in the original engine; private
exponential/GP selector headers plus reproducible constant generator; host-only
namespace bridge, real-processor/device benchmark, scientific comparison and
additional SPICE/ARM/preservation tools; immutable before/after observations and
new independent corner fixtures; CMake/CI to rerun and publish them; ADR/status/
registry/report to record the no-go honestly. Original GP header, public API,
RAT/CMOS DSP/tools, original SPICE fixtures, all component/failure definitions,
patch format and astrology rules remain unchanged. No new processor is added.

**Unfulfilled objective:** no accepted default-path acceleration or removal of
its expm1 calls. Mathematical H64/P12 equivalence passes the tested gates, but
host full-processor improvement does not. H32/P8 fail the predeclared mathematical
gates, and receive no full-matrix acceptance claim. Keep the high-precision
default and the experimental alternatives private. This is a measured negative
result, not a completed acceleration success.

Unproven: numerical behavior on an executing ARM CPU, M7 cycles/cache/stack,
hardware boot/audio/latency/noise/power, full physical transistor/pedal calibration,
real guitar and founder listening approval. No “inaudible”,24-bit-noise-floor or
universal part/model accuracy claim. Next smallest ticket: measure reference and
H64/P12 standalone kernels on a physical M7 with controlled DWT cycle counters,
cache settings and stack watermark, using identical stored input/corner traces,
before revisiting any evaluator default. No patch integration is needed for that
decision. PR remains open for independent review; do not merge automatically.
