# SF-TEST-PERF-001 — measured verification throughput repair

**All original scientific/safety/regression gates pass locally. The 3x and600s Debug
performance goals were NOT attained.** The best tested unchanged-flags increment
is 1.93x less full-Debug wall time, with one additional tool-contract test. This is
not a claim that every possible safe optimization is exhausted or that future
circuit assignments will finish in 40–50 minutes.

Main/base `af159d833bdd0b0e17ccc59fcf7edfa506ed98f5`; Gate0 commit `64db234`;
tested implementation `d05633fe4cea9ad36842fd4b452a3b94ce2caea1`. Subsequent documentation/evidence changes do not change the tested executable code,
CMake, workflows or executable tools.
Drafts #61/#63 remain unmerged/untouched; no FZ1/Blender scientific work imported.
Root AGENTS is absent; project AGENTS/Constitution/status/Soundforge/CMake/CI read.
[Gate0](SF_TEST_PERF_GATE0.md), [canonical profile v1](SOUNDFORGE_VERIFICATION_PROFILE.md).

## Actual complete pre/post results

Same Linux6.18.57/x86_64, GNU 14.2.0, CMake/CTest 3.31.6 and Python 3.12.14, pinned
public JSON 3.11.3 (45 cache headers checked against the SHA-pinned archive).
Affinity0,1; visible CPUs 3; cgroup CPU quota 2, memory 8 GiB. Fresh, separate build
and evidence directories for every phase/configuration. Exactly one full baseline
and one final full run per configuration; targeted checks during iteration.
Debug remains original -g/-O0; Release remains -O3/-DNDEBUG. ASan+UBSan is the
same Debug+-O1/non-PIE/leak/halt configuration before and after. No DSP compilation
flags changed. Configurations, ARM and TSan did not overlap timed full suites.
Short audit/archive operations occurred; timings are one observational trial on a
shared host, not calibrated WCET/confidence intervals or controlled cold-cache tests.
Both sides built fresh; no fewer cases, cached pass or old rendered output used.

Times below are monotonic around actual CTest; its own displayed totals differ
by~0.1s. CPU includes waited-for descendants, not just Python launch overhead.

|Configuration|Before tests|After tests|Before wall s|After wall s|Gain|Before CPU s|After CPU s|Observed pre+post test wall s|
|---|---:|---:|---:|---:|---:|---:|---:|---:|
|Debug|38/38|39/39|2459.219|1276.465|1.927x|2477.382|2531.767|3735.684|
|Release|38/38|39/39|233.104|129.098|1.806x|246.827|248.917|362.202|
|ASAN|38/38|39/39|2449.107|1272.007|1.925x|2468.081|2513.886|3721.115|

TSan actual activation/living contract/concurrency: **4/4,35.02s**. No ASan,
UBSan or TSan diagnostic observed. Existing38 cases remain unconditional; the
39th is a Python contract test that exercises actual CTest failure propagation,
launch-file restoration, stale-evidence rejection, isolation and artifact checksums.
Its three unit methods pass in each full configuration. No old timeout, fixture,
threshold, input duration, supported rate, callback guard, repeat or golden changed.

The actual pre+post sequence above pays the serial baseline measurement cost;
it is NOT a1.93x measured end-to-end engineering-workflow gain. Applying the new
schedule to both future unchanged-corpus Debug runs would forecast~42.5 minutes
for those two suites alone, not observed future delivery time. Build, scientific
research, new audio, review and publication add time. There is no40–50minute promise.

## Bottleneck, change and limits

The old workflow omitted CTest parallelism; no CTEST_PARALLEL_LEVEL override was
specified, so it ran one case at a time. It also built serially by default. The
four TS808/Klon cases consumed~79% of baseline Debug wall time. Six actual Debug
cases were sampled: those four, Muff science and wavefolder science. The sampled
call trees attribute~99% of TS808/Klon CPU to native process; matrix evaluation,
nonlinear solve, reconstruction and FIR dominate. Preparation~.02–.03%, Klon CSV
parsing~.08%, transient val conversion~.33–.41%; synthesis, guard, FFT, receipt IO
and subprocess setup are small. These are indicative sampled shares, not exact
phase accounting. gprofng 2.44 emits a timer-shutdown warning and its sample-time
scale disagrees with wait4; complete headers/function tables are retained and
sampler seconds are NOT used as authoritative time or projected savings.

- `tests/test_ts808.cpp` / `test_klon.cpp`: cache amplitude/frequency/shape once
  per fixture row, instead of repeatedly parsing the same strings per sample.
  Every native step, sample-level guard and CHECK expression remains. The tiny
  parsing change is not credited with an isolated measured CPU speedup: overall
  CPU time actually rose slightly in parallel runs. Wall gain is chiefly scheduling.
- `CMakeLists.txt`: measured-cost priorities on fresh builds, science/listening/
  safety labels and PROCESSORS2 for existing multithread stress tests and the new
  nested-CTest tool contract. All old 38 names/commands/gating properties preserved.
  Artifact destinations are build-local or already private TemporaryDirectory paths;
  no mutable source fixture/output is shared. No new resource lock enters DSP.
- `host-tests.yml`: explicit two-worker build/CTest, retaining all jobs, steps,
  filters, notices, full native renders/repeats and artifact publications. One
  Guitar-TECHS archive download replaces three per Release job. All three unchanged
  preparers independently verify the SAME archive SHA, selected source and prepared
  data; controls/takes/attribution unchanged. This is within-job file reuse, **not**
  a cross-run rendered cache or a cached scientific pass. No remote network-time
  improvement measured; no fresh local full archive download/preparation performed.
- `benchmark.py`, `negative_controls.py`, `test_soundforge_benchmark.py`: reusable
  actual full-suite wall/CPU/RSS/command measurement, read-only CLI digest observation,
  isolated deliberate corruption and executable failure-preservation checks.
  `SOUNDFORGE_VERIFICATION_PROFILE.md` centralizes the existing complete contract;
  its README link reduces specification drift, not coverage.

The complete38-case baseline Debug CPU total2477.382s gives a same-work ideal
2-core scheduling floor1238.691s. Actual final1276.465s is near that conditional
bound. A3x/819.740s or600s result would require substantially less computation,
not simply more scheduling on the same quota. This is NOT a theorem excluding
all future harness optimizations. No production math, O0->optimized Debug switch,
shorter stimulus/thinner fixture, omitted guard/repeat or cached physics result
was used to manufacture a gain. Altering the Debug build policy needs its own
explicit evidence/review; no alternative-mode gate replacement was investigated.
Splitting cases cannot remove retained native work and was unnecessary for this
schedule (longest case is below the two-core aggregate floor).

[Schedule estimates](data/sf-test-perf/schedule-estimates.json) replay all 38 measured
baseline durations: naive j2 **1288.809s**, cost/reserved j2 **1243.517s**. Both
are estimates ignoring contention/startup cost; naive also ignores stress-thread
oversubscription. The measured comparison is baseline serial j1 versus final
cost-aware j2, so it does not isolate the scheduler and parsing changes. A third
same-revision full scheduling A/B was NOT run, respecting the one-full-pre/post
limit. No estimate is called an actual third benchmark.

## Preserved proof, beyond source hashes

All427 tracked implementation/header/firmware/fixture files are SHA-identical.
All118 original TS808/Klon CHECK expressions are unchanged. Full executions retain
**TS808 14,053,681** and **Klon 12,614,277** checks in each before/after configuration.
All38 original CTest commands remain, with no label exclusions/WILL_FAIL/disabled
science. [Frozen assertions/fixtures/inventory and raw logs](data/sf-test-perf/).

Full original TS808 device/diode/bias, five transient grids192000/384000/768000/
1536000/705600, all 63 complex-AC rows,28 harmonic-reference rows, convergence,
1x/16x alias improvement>6dB and full rate/control/reset/rejection/invalid audio/
long-note/silence/guard checks execute. High-grid limits remain .01V aggregateRMS,
.1V node peak,.001V jack peak,50uA current; original coarser diagnostic budgets
and refinement ratios unchanged. All7196 rows in each transient file retained.

Klon device/bias, transient grids529200/576000/768000/1152000 (8260/8480/9380/11340
rows),120 harmonic rows, the original 54 selected AC conditions/378 complex signal
comparisons,1x/12x alias improvement>15dB,162 control endpoint/rate/fault combinations
and complete dynamic/guard/invalid/reset/silence coverage execute. Transient stage
RMS .01V, ordinary/harsh peaks .05/.2V, ordinary jackRMS .001V, jack peaks .005/.01V,
KCL1e-10A/path1e-5V and all original current/complex/convergence limits unchanged.
Independent committed SPICE references are compared by actual native execution;
unchanged scientific reference generators were not rerun or rewritten here.

- **24 electrical CSV comparisons** (four per model x two models x three configs)
  are byte-identical: transient accuracy, complex stage sweeps, harmonics and alias.
- **74 distinct native WAVs per configuration** are byte-identical before/after;
  counting TS808's required second full CLI render gives **300 WAV comparisons**
  across the three configurations. All127 non-timing file comparisons/config match.
  Only the preexisting nondeterministic host_processor_us receipt column is excluded;
  actual audio, levels, gains, currents, settings, traces, spectra and notes are retained.
  New real-DI audition packages were not generated locally; existing CI's complete
  real-guitar render/repeat/publication steps remain, with original rights notices.
- **27 Release production libraries/lab/demo binaries are entirely byte-identical**.
  The corresponding27 Debug runtime images also match after stripping only debug,
  build-ID and compiler-comment metadata. See native-binary-preservation.json.
- Existing LabRAT golden/output, patch, preset power-loss, activation, living,
  tape, rotary, CMOS, germanium, Muff and wavefolder regressions all execute/pass.

Three deliberately corrupted isolated copies trigger the ORIGINAL gates:
+1A independent TS808 base-current expectation; invalid WAV replacing the CLI's
generated source; one zeroed LabRAT golden digest. All **3/3 reject**, isolated
CTest exit8, at the expected diagnostics. No tracked reference/input/golden was
mutated. This is a targeted original-gate negative run, not a second complete
mutant regression suite; unchanged unconditional entrypoints and the new nested
CTest contract prove failures continue to propagate.

Per-test RSS high-water measurements are included in JSON, not claimed as aggregate
process-tree memory. The optional ASan tree observer recorded **728.465 MiB sumRSS**
(shared pages counted per process; preset-journal fork contributed). It missed
approximately22 initial seconds, so this is a sampled observed value, not a whole-run
peak guarantee, PSS or hard bound. Its original static before-CTest label was
incorrect and corrected from durations in sampled-memory.json. No baseline tree
peak was recorded; no apples-to-apples aggregate memory improvement is claimed.
The host limit was8 GiB; nothing here measures Daisy RAM/stack high-water.

## Actual ARM proof, not hardware performance

Pinned xPack 14.2.1-1.1 / libDaisy 1289a000bc29dae1bc0ed4129cfa2df40ef71574;
full firmware recipe exits0 after a clean SDK build, **61.948s**. Seven images
actually link. Existing internal-flash full-codec diagnostic still overflows
by **88,800B**, the expected non-gating probe; no linker region enlarged or rule
changed. All seven existing activation+living/tape/rotary/Muff/TS808/Klon/wavefolder
object/layout/call/stack audits exit0. [Full sections/pins/hashes](data/sf-test-perf/arm/).

|Linked image|.text bytes|.data bytes|.bss bytes|separate SRAM1 BSS bytes|
|---|---:|---:|---:|---:|
|minimal_internal_flash|93516|988|23388|16384|
|diagnostic_sram|212388|1380|23460|16384|
|diagnostic_qspi|212388|1380|23460|16384|
|rat_integration|93652|988|23388|16384|
|first_boot_led_recovery|49968|908|11964|0|
|first_boot_mono_bypass|107312|1564|36000|16384|
|first_boot_rat_composite|107496|1564|36000|16384|

Other sections/placement/stack and each ELF/bin checksum remain in the actual
report. No flashing, bootloader/hardware compatibility, measured Seed3 cycles,
IRQ/latency/noise or musical approval. Firmware/DSP/patch/contracts/correspondence
source paths are unchanged. Engineering verification does not approve a voice.

## Per-test timing table

Each cell is **before wall/CPU -> after wall/CPU seconds**. Individual wall time can
increase under concurrency; suite elapsed time is the throughput result. RSS and
full precision/actual commands are in per-test.json. New denotes the added tool gate.

|Exact CTest name|Debug|Release|ASan+UBSan|
|---|---:|---:|---:|
|firmware_measurement_math_status|0.00/0.00 -> 0.00/0.00|0.00/0.00 -> 0.00/0.00|0.01/0.01 -> 0.01/0.01|
|core_validation|0.00/0.00 -> 0.00/0.00|0.00/0.00 -> 0.00/0.00|0.01/0.01 -> 0.01/0.01|
|audio_safety|0.05/0.05 -> 0.05/0.05|0.01/0.01 -> 0.01/0.01|0.03/0.03 -> 0.03/0.03|
|patch_contract|1.09/1.09 -> 1.05/1.04|0.14/0.13 -> 0.13/0.12|1.86/1.86 -> 2.23/2.21|
|host_preparation|0.03/0.02 -> 0.03/0.02|0.01/0.00 -> 0.00/0.00|0.03/0.03 -> 0.04/0.03|
|rat_patch_integration|1.96/1.94 -> 1.88/1.87|0.79/0.79 -> 0.77/0.77|1.76/1.75 -> 1.68/1.67|
|patch_audio_evidence|6.66/6.61 -> 6.02/6.00|1.85/1.85 -> 1.88/1.87|6.25/6.22 -> 6.27/6.20|
|rat_circuit_and_safety|2.21/2.20 -> 2.16/2.13|0.57/0.57 -> 0.60/0.60|2.32/2.31 -> 2.22/2.21|
|rat_lab_measurements|3.46/3.45 -> 3.42/3.38|0.80/0.79 -> 0.82/0.82|3.38/3.37 -> 3.31/3.29|
|soundforge_utilities|1.15/1.14 -> 0.93/0.92|0.74/0.74 -> 0.79/0.78|0.90/0.89 -> 0.83/0.82|
|soundforge_registry|0.05/0.05 -> 0.04/0.04|0.04/0.04 -> 0.04/0.04|0.04/0.04 -> 0.04/0.04|
|soundforge_registry_negative|0.18/0.18 -> 0.15/0.15|0.15/0.15 -> 0.16/0.16|0.15/0.15 -> 0.16/0.16|
|labrat_output_preservation|18.66/18.54 -> 17.44/17.37|3.99/3.97 -> 4.17/4.16|17.46/17.37 -> 17.48/17.34|
|cmos_circuit_and_safety|1.20/1.19 -> 1.16/1.15|0.25/0.25 -> 0.27/0.25|1.24/1.23 -> 1.23/1.22|
|cmos_lab_measurements|3.94/3.93 -> 3.44/3.44|0.82/0.80 -> 0.82/0.81|3.82/3.80 -> 4.45/4.42|
|cmos_lab_cli|11.32/11.30 -> 10.70/10.66|2.62/2.62 -> 2.55/2.54|11.77/11.74 -> 12.22/12.13|
|germanium_circuit_and_safety|6.90/6.88 -> 6.84/6.81|1.50/1.49 -> 1.51/1.50|5.63/5.60 -> 5.86/5.82|
|germanium_lab_measurements|6.98/6.96 -> 6.65/6.59|1.60/1.59 -> 1.51/1.51|5.12/5.10 -> 5.71/5.68|
|germanium_lab_cli|13.31/13.28 -> 13.59/13.58|3.19/3.19 -> 3.24/3.21|10.45/10.42 -> 10.88/10.81|
|preset_journal|17.10/17.06 -> 17.40/17.31|3.65/3.63 -> 3.66/3.65|32.86/32.84 -> 33.54/33.34|
|activation_contract|0.18/0.17 -> 0.17/0.16|0.09/0.09 -> 0.09/0.09|0.18/0.18 -> 0.19/0.19|
|activation_concurrency|26.08/43.65 -> 25.39/42.70|20.22/33.77 -> 20.45/34.07|25.74/43.93 -> 24.36/41.46|
|activation_demo|0.03/0.03 -> 0.04/0.03|0.02/0.02 -> 0.02/0.02|0.04/0.04 -> 0.05/0.05|
|living_contract|1.02/1.01 -> 0.95/0.94|0.20/0.20 -> 0.23/0.23|0.95/0.95 -> 0.98/0.97|
|living_concurrency|2.55/5.06 -> 2.34/4.64|0.47/0.91 -> 0.49/0.93|2.48/4.87 -> 2.50/4.92|
|living_audio_evidence|6.30/6.29 -> 6.19/6.15|1.69/1.69 -> 1.95/1.92|5.82/5.81 -> 5.97/5.96|
|tape_medium_and_safety|0.13/0.13 -> 0.12/0.12|0.03/0.03 -> 0.04/0.04|0.17/0.17 -> 0.15/0.14|
|tape_listening_cli|1.60/1.59 -> 1.57/1.55|0.47/0.47 -> 0.48/0.47|1.82/1.82 -> 1.79/1.78|
|rotary_mechanics_and_safety|3.61/3.61 -> 4.03/4.01|1.01/1.01 -> 1.04/1.04|2.36/2.35 -> 2.34/2.34|
|rotary_listening_cli|4.01/4.01 -> 4.15/4.14|1.19/1.19 -> 1.16/1.16|3.24/3.23 -> 2.96/2.96|
|muff_circuit_and_safety|164.91/164.62 -> 164.78/164.25|14.39/14.35 -> 14.75/14.73|150.16/150.00 -> 153.97/153.04|
|muff_listening_cli|187.52/187.39 -> 192.13/190.80|16.87/16.84 -> 17.99/17.88|174.05/173.90 -> 179.83/178.55|
|ts808_circuit_and_safety|510.31/509.78 -> 531.64/529.46|39.21/39.17 -> 36.54/36.48|501.66/501.47 -> 513.05/510.45|
|ts808_listening_cli|556.45/556.23 -> 575.11/571.98|38.11/38.09 -> 38.92/38.69|535.90/535.56 -> 548.82/546.07|
|klon_circuit_and_safety|390.04/389.90 -> 402.41/400.87|32.23/32.20 -> 32.76/32.67|410.49/410.40 -> 417.81/415.88|
|klon_listening_cli|473.06/472.86 -> 483.01/480.56|37.18/37.15 -> 38.18/38.05|492.50/492.20 -> 507.65/504.93|
|wavefolder_circuit_and_safety|16.39/16.35 -> 17.56/17.50|2.80/2.80 -> 2.81/2.78|17.63/17.62 -> 17.44/17.36|
|wavefolder_listening_cli|17.23/17.21 -> 17.39/17.30|2.81/2.80 -> 2.81/2.79|17.43/17.38 -> 17.52/17.45|
|soundforge_benchmark_contract|new -> 0.51/0.55|new -> 0.44/0.49|new -> 0.42/0.45|

## Exact reproduction

Actual original absolute-path orchestration/build logs, full CTest inventories,
per-test statuses, closed sampler headers, input/dependency/source/fixture hashes,
negative controls and every final audit argument/status are retained in data/.
The original baseline measurement wrapper was created outside the checkout before
implementation; the new reusable runner preserves the same native commands and
wait4 timing semantics, adds tested isolation/restoration and rejects stale evidence.

For a new same-machine comparison, use the same compiler and two allowed CPU IDs
on both sides. This complete recipe uses separate detached source/build directories.
Binary metadata can embed different source paths in that recipe; the exact binary
identity checks reported above used the same original source path before/after.
Run from the PR checkout's repository root with CMake/CTest3.31.6 on PATH:

```sh
git worktree add --detach ../parable-perf-before af159d833bdd0b0e17ccc59fcf7edfa506ed98f5
for sf_perf_phase in before after; do
  sf_perf_source=parable-starter
  sf_perf_jobs=2
  if [ "$sf_perf_phase" = before ]; then
    sf_perf_source=../parable-perf-before/parable-starter
    sf_perf_jobs=1
  fi
  cmake -S "$sf_perf_source" -B "build-$sf_perf_phase-debug" -DCMAKE_BUILD_TYPE=Debug
  cmake --build "build-$sf_perf_phase-debug" -j2
  taskset -c 0,1 python3 parable-starter/tools/soundforge/benchmark.py --build "build-$sf_perf_phase-debug" --output "evidence-$sf_perf_phase-debug" --jobs "$sf_perf_jobs" --profile
  cmake -S "$sf_perf_source" -B "build-$sf_perf_phase-release" -DCMAKE_BUILD_TYPE=Release
  cmake --build "build-$sf_perf_phase-release" -j2
  taskset -c 0,1 python3 parable-starter/tools/soundforge/benchmark.py --build "build-$sf_perf_phase-release" --output "evidence-$sf_perf_phase-release" --jobs "$sf_perf_jobs"
  cmake -S "$sf_perf_source" -B "build-$sf_perf_phase-asan" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -O1" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined -no-pie"
  cmake --build "build-$sf_perf_phase-asan" -j2
  ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 taskset -c 0,1 python3 parable-starter/tools/soundforge/benchmark.py --build "build-$sf_perf_phase-asan" --output "evidence-$sf_perf_phase-asan" --jobs "$sf_perf_jobs"
done
```

`--profile` needs host gprofng 2.44; no profiler is required for default full CTest.
Here CMake/CTest came from /workspace/work/baseline-tools/bin and all six builds
used -DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src,
verified unmodified public3.11.3 headers, not draft61 code. Exact actual dirs are
/workspace/work/sf-test-perf-{baseline,final}-{Debug,Release,ASAN}. Final full
commands exit0. Standard unfiltered `ctest --test-dir BUILD -j2 --output-on-failure
--no-tests=error` retains all gates without measurement instrumentation.

```sh
cmake -S parable-starter -B build-tsan -DCMAKE_BUILD_TYPE=Debug   -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer -fno-pie -O1"   -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread -no-pie"
cmake --build build-tsan --target parable_activation_test parable_living_test -j2
TSAN_OPTIONS=halt_on_error=1 ctest --test-dir build-tsan -j2 --output-on-failure   --no-tests=error -R '^(activation|living)_(contract|concurrency)$'
python3 parable-starter/tools/soundforge/negative_controls.py   --project parable-starter --build build-after-release --output negative-evidence
python3 parable-starter/firmware/build.py --cache /ABS/PINNED/CACHE --output arm-proof
# Each existing check_{activation,tape,rotary,muff,ts808,klon,wavefolder}_arm.py
# runs with --compiler /ABS/PINNED/CACHE/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++
# and a distinct --output; activation additionally uses --living.
```

Every actual full ARM/TSan/negative-control argument and elapsed/exit result is
in final-audits/summary.json. YAML parsed and exact original job/step/filter/artifact/
notice-command preservation checked except declared j2/archive edits. Tracked
symlinks and diff whitespace checked. Unchanged fixture/notice/licensing eligibility
preserved; no new DSP import. Physical reference generators were not restarted.

## Unfulfilled conditions and next smallest action

- **3x and<=600s full-Debug goals not achieved** at unchanged Debug flags on 2 CPUs.
  The result is 1.93x,21.27 minutes, with all gates retained. No universal-optimality claim.
- Separate same-revision plain-j2 versus resource-j2 full A/B not executed; the
  replay is explicitly estimated. Exact per-phase CPU split/sampler calibration
  and baseline aggregate memory peak remain unmeasured.
- Cross-run rendered-artifact/input caching is not implemented; old CI packages
  still render/repeat. Safe reuse needs complete source/compiler/dependency/input/
  executable/runtime/CPU identity, output checksums and executable preservation,
  including negative invalidation tests. No cached pass substitutes for science.
- New remote CI execution status is in the PR. Known Actions startup failure is
  separate; an unstarted job is not a code failure or remote pass. No required
  check/permission/branch protection/account setting was removed or bypassed.
- No Daisy execution or new founder audition; this is verification infrastructure.

**Next smallest action: SF-TEST-PERF-002 — validated old-audition reuse.** Measure
CI render cost, add only content-addressed unchanged-audition reuse with complete
runtime/input/toolchain fingerprints and executable/hash invalidation tests, while
keeping current-head complete scientific/regression/sanitizer/ARM execution.
Review the 3x/10minute goal against measured fixed-work/host bounds before promising
40–50minute future circuit deliveries. Do not reopen61/63 science as part of this.
