# SF-TEST-PERF-001 Gate 0 — main, measurement and immutable gates

Current main `af159d833bdd0b0e17ccc59fcf7edfa506ed98f5`; issue64; drafts61/63
are open and are not imported, modified, merged or relied on. No circuit research
is restarted. Root AGENTS is absent; `parable-starter/AGENTS.md`, Constitution,
README/status, Soundforge recipe, actual main CMake and both CI workflows read.
This audit is committed before implementation.

## Actual clean baseline, before repository edits

Linux6.18.57/x86_64, GNU14.2.0, CMake/CTest3.31.6, Python3.12.14. CPU affinity0,1
on a host exposing three CPUs but cgroup quota200000/100000 (two CPUs), memory8GiB.
One clean build and complete unfiltered run per configuration; no overlap between
configuration runs. Debug/Release use original flags (-g/-O3 -DNDEBUG respectively).
Full ASan+UBSan uses Debug plus -O1, non-PIE and leak/halt flags. Same flags are
required after changes; no DSP optimization flags will be changed.

|Configuration|All tests|Suite wall seconds|User+system CPU seconds|Same-work CPU/2 floor|
|---|---:|---:|---:|---:|
|Debug|38/38|2459.219|2477.382|1238.691|
|Release|38/38|233.104|246.827|123.413|
|ASAN|38/38|2449.107|2468.081|1234.040|

The CPU/2 column is an ideal same-work scheduling bound, not a promise that no
harness work can be removed. In particular, 3x and600s are goals, not a license
to change the debug DSP flags, omit cases or replace actual deterministic renders.

Each test below is wall/CPU seconds. POSIX wait4 includes waited-for CLI descendants;
CTest itself is the actual executor. A transparent timing wrapper records process
status/RSS without replacing assertions. Debug's six expensive cases additionally
use the same gprofng collector planned for post-edit measurements. Its shutdown
timer warning and sample-seconds discrepancy make its **relative hot-path shares
indicative only**; authoritative times are wait4/monotonic. No sampler-derived
absolute CPU saving or exact solver-only time is claimed.

|Exact CTest name|Debug wall/CPU|Release wall/CPU|ASan+UBSan wall/CPU|
|---|---:|---:|---:|
|firmware_measurement_math_status|0.00/0.00|0.00/0.00|0.01/0.01|
|core_validation|0.00/0.00|0.00/0.00|0.01/0.01|
|audio_safety|0.05/0.05|0.01/0.01|0.03/0.03|
|patch_contract|1.09/1.09|0.14/0.13|1.86/1.86|
|host_preparation|0.03/0.02|0.01/0.00|0.03/0.03|
|rat_patch_integration|1.96/1.94|0.79/0.79|1.76/1.75|
|patch_audio_evidence|6.66/6.61|1.85/1.85|6.25/6.22|
|rat_circuit_and_safety|2.21/2.20|0.57/0.57|2.32/2.31|
|rat_lab_measurements|3.46/3.45|0.80/0.79|3.38/3.37|
|soundforge_utilities|1.15/1.14|0.74/0.74|0.90/0.89|
|soundforge_registry|0.05/0.05|0.04/0.04|0.04/0.04|
|soundforge_registry_negative|0.18/0.18|0.15/0.15|0.15/0.15|
|labrat_output_preservation|18.66/18.54|3.99/3.97|17.46/17.37|
|cmos_circuit_and_safety|1.20/1.19|0.25/0.25|1.24/1.23|
|cmos_lab_measurements|3.94/3.93|0.82/0.80|3.82/3.80|
|cmos_lab_cli|11.32/11.30|2.62/2.62|11.77/11.74|
|germanium_circuit_and_safety|6.90/6.88|1.50/1.49|5.63/5.60|
|germanium_lab_measurements|6.98/6.96|1.60/1.59|5.12/5.10|
|germanium_lab_cli|13.31/13.28|3.19/3.19|10.45/10.42|
|preset_journal|17.10/17.06|3.65/3.63|32.86/32.84|
|activation_contract|0.18/0.17|0.09/0.09|0.18/0.18|
|activation_concurrency|26.08/43.65|20.22/33.77|25.74/43.93|
|activation_demo|0.03/0.03|0.02/0.02|0.04/0.04|
|living_contract|1.02/1.01|0.20/0.20|0.95/0.95|
|living_concurrency|2.55/5.06|0.47/0.91|2.48/4.87|
|living_audio_evidence|6.30/6.29|1.69/1.69|5.82/5.81|
|tape_medium_and_safety|0.13/0.13|0.03/0.03|0.17/0.17|
|tape_listening_cli|1.60/1.59|0.47/0.47|1.82/1.82|
|rotary_mechanics_and_safety|3.61/3.61|1.01/1.01|2.36/2.35|
|rotary_listening_cli|4.01/4.01|1.19/1.19|3.24/3.23|
|muff_circuit_and_safety|164.91/164.62|14.39/14.35|150.16/150.00|
|muff_listening_cli|187.52/187.39|16.87/16.84|174.05/173.90|
|ts808_circuit_and_safety|510.31/509.78|39.21/39.17|501.66/501.47|
|ts808_listening_cli|556.45/556.23|38.11/38.09|535.90/535.56|
|klon_circuit_and_safety|390.04/389.90|32.23/32.20|410.49/410.40|
|klon_listening_cli|473.06/472.86|37.18/37.15|492.50/492.20|
|wavefolder_circuit_and_safety|16.39/16.35|2.80/2.80|17.63/17.62|
|wavefolder_listening_cli|17.23/17.21|2.81/2.80|17.43/17.38|

## Bottleneck and schedule audit

The main workflow specifies no -j and no CTEST_PARALLEL_LEVEL: CTest defaults to
one test at a time. Its build is also serial by default. Current38 commands,
properties and unique evidence destinations are in the inventory JSON. Listening
CLI tests use private TemporaryDirectory paths; TS808/Klon measurements and
patch/living evidence already use distinct build-local subdirectories. Separate
build directories isolate configurations. No mutable committed fixture is shared.

TS808 circuit510.31s/CLI556.45s; Klon circuit390.04s/CLI473.06s account for~79% of
Debug. Muff circuit164.91s/CLI187.51s is the next large contributor. Sampled
call trees point to native process/sample, nonlinear evaluation/solve, state
reconstruction and FIR; preparation, synthesis, fixture IO, guards, FFT and receipts
are minor by comparison. Individual TS808/Klon transient loops nevertheless call
std::stod on the same amplitude/frequency/shape strings for every native sample.
This is demonstrably avoidable test-harness computation; parse once per fixture
row, retaining every sample and assertion. No scientific or safety guard is removed.
No exact per-phase CPU apportionment is inferred from these limited samples.

Pre/post compare actual full-suite serial -j1 against bounded -j2 with longest-cost
first and two-worker reservations for multithreaded stress tests. A replay of
measured per-test CPU durations will compare naive2-slot and cost-aware schedules;
that replay is an estimate, not a third measured full suite. This respects the
issue's one full pre/post run per configuration limit. The observed combined
improvement must not be presented as an isolated scheduler-only A/B.

## Frozen proof and permitted changes

- All427 tracked src/include/firmware/fixtures files hashed; no production DSP,
  circuit constants, fault, correspondence, firmware, contract or reference changes.
- TS80864 CHECK expressions /14,053,681 executed checks and Klon54 expressions /
  12,614,277 executed checks in each baseline configuration. Preserve all original
  expressions, loop lengths, supported rates, tolerances and executed check counts.
- TS808 transient7196rows per file over all five original grids, device27/diode33/
  bias6/AC63/harmonic28 rows; Klon transient8260+8480+9380+11340 rows on four grids,
  device101/bias25/harmonics120/AC378 rows (original54 selected conditions and378
  complex-signal comparisons). No fixture, endpoint, source-independent assertion,
  convergence ratio, harmonic or alias requirement changes.
- Original26 TS808 /48 Klon CLI WAVs, every raw/matching/current/KCL/PCM/path/guard/
  reset/rejection/hardlink and deterministic property remain. Read-only observation
  hashes actual outputs before existing cleanup; only the explicitly nondeterministic
  host_processor_us column is excluded, as in prior policy. Keep LabRAT old goldens.
- All other36 baseline commands/assertions remain; one small timing-tool contract
  may be added unconditionally. Full ASan/UBSan, existing TSan and full pinned ARM
  recipe/audits are required. CI startup failure is separate; no checks bypassed.

Small proposed increment: hoist repeated transient parameter conversions in two
harnesses; explicit two-job CI build/test schedule and measured costs/reservations;
reuse one checksum-verified Guitar-TECHS archive for the unchanged three preparers;
add reproducible timing/negative-control tools and one canonical verification
profile. Keep all actual old audio renders/repeats; no rendered pass/artifact cache.
All source/license notices and input preparers remain unchanged. Other possible
optimizations are rejected unless measurement proves both benefit and identical proof.

Raw per-test measurements, complete inventory/commands, source/test/fixture/threshold
hashes, sampler warnings, exact baseline orchestration and emitted check counts
will accompany SF_TEST_PERF_REPORT.md. Known remote Actions startup failure remains
an external infrastructure limit; no unstarted run is called a scientific failure.
