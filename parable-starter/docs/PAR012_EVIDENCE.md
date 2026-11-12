# PAR-012 — reproduction, evidence and limits

Base main `8f943681ad933f7d8b65d1d66535aa811c75c48e`, including merged #33;
issue #34 and founder upload govern scope. Issue #39 is compatibility reference
only. [Gate 0](PAR012_GATE0.md) preceded source edits. Required project files
were present; root AGENTS.md is absent (rules are under `parable-starter/`).

## Changes and reuse

| Files (project-relative unless `.github`) | Reason |
| --- | --- |
| `include/parable_living.h`, `src/parable_living{,_audio}.cpp` | Fixed typed source contract, bounded control scheduler and SPSC snapshots; two explicitly test-only gain destinations around public Runtime. |
| `tools/living_fixture.h`, `tests/fixtures/living/trace-{a,b}.csv` | Transparent synthetic heterogeneous cadence adapter and bounded file replay; explicit normalized test controls/consent scenarios. |
| `tools/living_demo.cpp` | Same canonical document saved/loaded through existing simulated journal, then actual Runtime/Engine WAVs and observation/control/permission traces. |
| `tests/test_living.cpp`, `tests/activation_guard.cpp` | Adversarial source/timing/unit/consent/overflow tests, actual DSP unity/ramp/replay/concurrency reference checks; complete nothrow allocation interposition in reused guard. |
| `CMakeLists.txt`, `.github/workflows/{host-tests,arm-firmware}.yml`, `tools/check_activation_arm.py` | Three added host tests, sanitizer coverage, repeatable listening/benchmark artifacts and optional living ARM audit. Original firmware targets stay unchanged. |
| Requirements/Gate 0/this report/source manifest, README/status/ADR-001 pointers | Sourced availability matrix, lifecycle/policy, measured evidence, outstanding gates. |

Reuse PAR-010 canonical storage, PAR-011 decode/prepare/ownership lifecycle,
unchanged Engine/RAT and prior SPSC acquire/release pattern. Reuse Soundforge
owned synthetic plucks, statistics and PCM16 writer, Linux callback guard and
pinned ARM inspection tool. No new dependencies, DSP algorithms, canonical fields,
correspondence rules, hardware adapter or storage changes. Persistent source
mappings/processor automation are **not** designed by these test bindings.

## Exact build/test commands

Observed host: Linux x86_64, GNU C++ 14.2.0, CMake/CTest 3.31.6, Python 3.12.14.
From the repository root (CMake on PATH):

```bash
export PATH=/workspace/work/baseline-tools/bin:$PATH
cmake -S parable-starter -B /workspace/work/par012-before-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/par012-before-Debug -j2
ctest --test-dir /workspace/work/par012-before-Debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /workspace/work/par012-before-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/par012-before-Release -j2
ctest --test-dir /workspace/work/par012-before-Release --output-on-failure --no-tests=error -j2
```

Those directories were absent for baseline builds; both **23/23** passed before
edits (118.28 / 78.84 s). They were then incrementally rebuilt for final tests.
On another machine, substitute fresh writable build paths; no workspace cache is
required. First configure downloads the already pinned nlohmann-json dependency.
No tests use disappearing Release assertions.

Sanitizers (same flags and target selections as CI):

```bash
cmake -S parable-starter -B /workspace/work/par012-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build /workspace/work/par012-asan --target parable_core_test parable_audio_safety_test parable_patch_test parable_prepare_test parable_rat_test parable_rat_integration_test parable_patch_audition parable_firmware_measurement_test parable_presets_test parable_activation_test parable_activation_demo parable_living_test parable_living_demo -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir /workspace/work/par012-asan --output-on-failure --no-tests=error \
  -R '^(firmware_measurement_math_status|core_validation|audio_safety|patch_contract|host_preparation|rat_patch_integration|patch_audio_evidence|rat_circuit_and_safety|preset_journal|activation_contract|activation_concurrency|activation_demo|living_contract|living_concurrency|living_audio_evidence)$'
cmake -S parable-starter -B /workspace/work/par012-tsan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=thread -fno-omit-frame-pointer -fno-pie -O1' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=thread -no-pie'
cmake --build /workspace/work/par012-tsan --target parable_activation_test parable_living_test -j2
TSAN_OPTIONS=halt_on_error=1 ctest --test-dir /workspace/work/par012-tsan \
  --output-on-failure --no-tests=error -R '^(activation|living)_(contract|concurrency)$'
```

Final local results (all commands exited 0; no compiler warnings):

| Check | Result | CTest wall time |
| --- | --- | ---: |
| Complete Debug | 26/26 | 170.43 s |
| Complete Release (`-O3 -DNDEBUG`) | 26/26 | 89.87 s |
| Targeted ASan/UBSan, leak detection on | 15/15 | 235.01 s |
| Actual activation + living contract/concurrency TSAN | 4/4 | 120.28 s |
| Pinned ARM compile/layout/callback-symbol audit | pass | not a runtime benchmark |
| Complete demo, chunks 1/7/256 | all files identical | not a runtime benchmark |

Host times include contention from concurrent test/build processes; they are not
deadline estimates. CI execution/results and artifact links are recorded in the
PR after publication. Initial ASan
found `operator new`/`free` family mismatch in the reused guard: libstdc++ replay
sorting used the missing nothrow overload. Added matching scalar/array/aligned
nothrow allocation/deletion and a self-test; **no sanitizer suppression**. This
also closes a callback allocation-detection blind spot. No production allocator
or sorting in audio was introduced.

## Listening and deterministic replay

```bash
/workspace/work/par012-before-Release/parable_living_demo --output /workspace/work/par012-demo --chunk 7
/workspace/work/par012-before-Release/parable_living_demo --output /workspace/work/par012-repeat-1 --chunk 1
/workspace/work/par012-before-Release/parable_living_demo --output /workspace/work/par012-repeat-256 --chunk 256
diff -rq /workspace/work/par012-demo /workspace/work/par012-repeat-1
diff -rq /workspace/work/par012-demo /workspace/work/par012-repeat-256
/workspace/work/par012-before-Release/parable_living_test
/workspace/work/par012-before-Release/parable_living_test --stress
```

Four 9 s/48 kHz PCM16 files: `00_input`, `01_unmodulated`, `02_pre_test`,
`03_post_test`. The three processed runs use the **same saved.patch.json**, from
one journal save/power-cycle/load. One Runtime publication per render, stable
receipt token throughout; source updates do not reprepare/reload the graph.
Raw levels, no RMS matching. Synthetic plucks and all source values are generated
or explicitly enumerated non-personal stimuli; no observed wearable or real guitar.

Measured float results before PCM16 quantization:

| Comparison/render | RMS difference from unity | Peak | RMS |
| --- | ---: | ---: | ---: |
| Pre-gain fixture A | 0.0104718481418 | 0.0882118716836 | 0.0336826481469 |
| Post-gain fixture B | 0.0172020094243 | 0.0880091562867 | 0.0262731766734 |

A versus B RMS difference **0.0178934700288**. Unity max absolute error **0**
against independently loaded unchanged native Engine; adversarial unity samples
also compare binary32 bytes. New unity WAV matches the prior canonical composite
WAV byte for byte. All eight prior canonical WAV/patch files per Debug/Release
match the pre-edit SHA256 manifest; old LabRAT goldens remain enforced by CTest.
Entire demo output (WAV, canonical document, reports and all traces) matches
across ingestion chunks **1 / 7 / 256**, including dropout and permission actions.
Numerical distinction and listening artifacts are proven; founder listening/
musical quality/physiological causation are **not auditioned or approved**.

`*-observations.csv` preserves all consumed source facts, including original
measurement/arrival time, mapped time/delay, units, origin, quality, confidence,
permission, sequence and XYZ values. `*-controls.csv` gives each 10 ms tick's
actual patch token/epoch, availability, known-age flags, quality, requested target
and current slewed gain. Missing timestamps/values must be interpreted using those
flags/states, not numeric placeholders. `*-permissions.csv` logs explicit scenario
actions. `LISTEN.txt` identifies exact dropout/recovery intervals and enum lookup.

Release CI artifact **parable-living-source-evidence** (30 days) publishes WAVs,
saved document, traces, source hashes/commit/compiler, checksums and raw measured
layout/throughput/concurrency logs. It repeats the complete demo at chunks 1/256
and diffs against default 7 before adding provenance. Run/artifact links and any
CI failures are recorded in the unmerged PR; local success is not a CI claim.

## Bounded resource and performance evidence

Actual ABI sizes (not the issue's hypothetical 24/32-byte wire records):

| Value/object | x86_64 bytes | pinned M7 bytes |
| --- | ---: | ---: |
| Record / profile / latest view | 64 / 112 / 104 | 64 / 112 / 104 |
| Snapshot / 32-entry Lane | 40 / 1,312 | 40 / 1,312 |
| Scheduler, including 256-entry raw FIFO | 24,544 | 24,536 |
| Audio wrapper, including two 1024-float arrays | 8,280 | 8,272 |
| Existing four-bundle Runtime | 45,288 | 45,224 |
| Combined objects | 79,424 | 79,344 |

The host raw FIFO/profile scheduler need not run on the pedal. Lane+wrapper are
**9,584 ARM bytes**, in addition to Runtime; no placement/integration selected.
These are owned object sizes, not total application heap/stack, firmware BSS,
transport encoding or a claim of measured target fit. Host fixture vectors are
separate: generation <=30 s; CSV <=1 MiB/20,000 rows/256-byte lines (bounded read
before allocation), vector capacity <=32,768 records (~2 MiB) per loaded set.
Demo output buffers/statistics/excitation and sanitizer/thread stacks add host RAM.

Representative measured **Release** control-lane run (including validation,
ingest, tick scan, test checks and guarded zero-frame audio consumption; no
observer/file/network/real DSP processing in this throughput timer):

| Stimulus | Records / bytes | Logical duration | Host wall µs | FIFO high water / dropped |
| --- | ---: | ---: | ---: | ---: |
| 32 KiB/s sustained equivalent | 5,120 / 327,680 | 10 s | 188.434 | 6 / 0 |
| 128 KiB/s burst equivalent, paced 10 ms ticks | 2,048 / 131,072 | 1 s | 67.232 | 21 / 0 |
| 128 KiB/s concentrated 250 ms batch | 512 offered / 32,768 B | 0.25 s | not separately timed | 256 accepted / 256 drop-new |

This is deterministic accelerated **host event processing**, not measured network
throughput or a 10-second real sensor recording. Figures vary with host load;
raw run log is published. No BLE framing, encryption, radio retransmissions,
phone battery or Daisy cycles are included. Normal advance handles <=256 events
and 16 profiles; simultaneous consent expiry can additionally compact <=256 FIFO
entries per affected profile (<=16). Observer work is caller-owned/control-only.
No unbounded archive, graph, queue growth or callback loops exist.

Concurrent test: **8,192 publications**, 56 epoch invalidations and four externally
joined restarts. Each audio block checks coherent paired snapshot fields, finite
bounded output and sample-exact actual RAT/reference Engine plus independent
slew computation. Reused callback guard must remain clean. Producer retries and
deadline checks live outside audio. The contract test separately forces full raw
and snapshot queues; concurrency does not promise a particular scheduling order.

## ARM compile audit, not execution

```bash
python3 parable-starter/tools/check_activation_arm.py --living \
  --compiler /workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ \
  --output /workspace/work/par012-arm-audit
```

To provision elsewhere, run existing pinned `python3 parable-starter/firmware/build.py`
then use `parable-starter/build/arm-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++`.
Toolchain/SDK pins and seven firmware targets are unchanged. CI runs this actual
audit after the existing firmware build and publishes it in **seed3-arm-build-evidence**.

Cortex-M7 hard-float, C++17 -O2, pinned GCC 14.2.1; callback compiled with
`-fno-exceptions -fno-rtti`. Living control object **4,172 text / 0 data / 0 BSS**;
living audio **1,396 / 0 / 0** before link. Original activation objects remain
**4,949 / 712 text**. Audio undefined symbols are **only Runtime::processBlock
and memset**; transitive Runtime audit permits only unchanged RAT::process and
memset. Disassembly has aligned uint32 loads/stores + DMB, no LDREX/STREX retries,
atomic helpers, allocation, locks, parse, circuit configuration or IO.
Direct static frame: wrapper processBlock **128 B**, Scheduler.advance **128 B**,
Lane.publish **28 B**, existing Runtime.processBlock **72 B**. These are not
cumulative stack/IRQ high-water or measured WCET. Zero object BSS does not remove
caller-allocated state. JSON/control math are outside the callback.

## Unfulfilled product gates and next step

No physical sensor/consent SDK, timestamp calibration measurement, wearable
background/guitar-playing dropout evidence, network transport, phone interface,
hardware firmware integration, flash/audio coexistence, M7 deadline/cache/ISR/
memory-placement proof, real guitar or founder audition. No PPG access is
fabricated; no Body Language or physiological-to-musical rule is implemented.
Persistent mapping schema and safe per-processor automation remain open decisions.

PAR-012's host engineering acceptance is distinct from those intentionally excluded
gates. Next smallest step: independently review this lane, then PAR-013 #38's
neutral guitar observation traces. Founder grammar and actual source adapter
authorization/clock/quality evidence need separate tickets. Leave this PR unmerged.
