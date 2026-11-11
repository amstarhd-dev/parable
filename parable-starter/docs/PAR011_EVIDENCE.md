# PAR-011 evidence and reproduction

Base: main `c288eb196fc51f4b49627784e427e8c76be4c91c`, merged PR #31.
[Gate 0](adr/ADR-001-par011-gate0.md) preceded code. [Protocol/API and limitations](PREPARED_ACTIVATION.md).
Linux x86-64, GCC 14.2.0, CMake 3.31.6, Python 3.12.14. No new fetched dependency.

## Exact local build/test commands

From repository root (CMake on PATH); substitute local output directories on a
fresh checkout. Baseline used the same three commands with `par011-before-Debug`
and `par011-before-Release` **before edits**:

```bash
cmake -S parable-starter -B /workspace/work/par011-after-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/par011-after-Debug -j2
ctest --test-dir /workspace/work/par011-after-Debug --output-on-failure --no-tests=error
cmake -S parable-starter -B /workspace/work/par011-after-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/par011-after-Release -j2
ctest --test-dir /workspace/work/par011-after-Release --output-on-failure --no-tests=error
cmake -S parable-starter -B /workspace/work/par011-sanitize -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build /workspace/work/par011-sanitize --target parable_core_test \
  parable_audio_safety_test parable_patch_test parable_prepare_test parable_rat_test \
  parable_rat_integration_test parable_patch_audition parable_firmware_measurement_test \
  parable_presets_test parable_activation_test parable_activation_demo -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir /workspace/work/par011-sanitize --output-on-failure --no-tests=error \
  -R '^(firmware_measurement_math_status|core_validation|audio_safety|patch_contract|host_preparation|rat_patch_integration|patch_audio_evidence|rat_circuit_and_safety|preset_journal|activation_contract|activation_concurrency|activation_demo)$'
cmake -S parable-starter -B /workspace/work/par011-tsan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=thread -fno-omit-frame-pointer -fno-pie -O1' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=thread -no-pie'
cmake --build /workspace/work/par011-tsan --target parable_activation_test -j2
TSAN_OPTIONS=halt_on_error=1 ctest --test-dir /workspace/work/par011-tsan \
  --output-on-failure --no-tests=error -R '^activation_(contract|concurrency)$'
/workspace/work/par011-after-Release/parable_activation_test --stress /workspace/work/par011-stress-final.csv
/workspace/work/par011-after-Release/parable_activation_demo 0 2 1 3 0 > /workspace/work/par011-demo.txt
/workspace/work/par011-after-Release/parable_activation_demo 0 2 1 3 0 > /workspace/work/par011-demo-repeat.txt
cmp /workspace/work/par011-demo.txt /workspace/work/par011-demo-repeat.txt
python3 parable-starter/tools/check_activation_arm.py \
  --compiler /workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ \
  --output /workspace/work/par011-arm-audit
git diff --check
```

The compiler path is the previously checksum-verified toolchain pinned in
`firmware/dependencies.json`; `firmware/build.py` bootstraps/verifies it on a new
Linux x86-64 environment. The audit script checks its version, compiles only the
new objects, and emits layouts, sizes, source hashes, exact commands, static frames
and actual audio disassembly/undefined symbols. It does not link/flash new firmware.
CMake registers guarded activation tests on Linux GNU/Clang; library/demo are
portable C++17. No unsupported platform's guard coverage is claimed.

## Results

| Check | Actual result |
| --- | --- |
| Clean baseline | 20/20 Debug, 94.51 s; 20/20 Release, 23.28 s |
| Complete final regression | 23/23 Debug, 192.47 s; 23/23 Release, 106.27 s |
| Targeted ASan + UBSan, leaks/halt enabled | 12/12, 162.71 s |
| Actual ThreadSanitizer, halt enabled | 2/2, 95.06 s; no race report |
| Engine audio preservation | 8 WAV/patch files per configuration byte-identical to baseline |
| Deterministic demo repeat | cmp exit 0 |
| Pinned ARM object/layout audit | Passed; not firmware/runtime integration |

Timings reflect overlapping local builds/tests, not processor benchmarks or CI
predictions. All pre-existing tests retained: patch goldens, journal/power cuts,
RAT/CMOS/germanium circuit/lab/CLI and output-preservation suites. Final builds have
no new compiler warnings. No failed test/acceptance criterion was waived.

Each stress run performs **4,096 successful publications**, four joined epochs,
randomized four-slot selection and 1..64-frame blocks. Release: **4,092 adoptions,
56,359 Busy attempts, 4,088 explicit retirement collections, 263,604 frames**.
ASan/UBSan: 4,092 / 118,632 / 4,088 / 253,409. TSAN: 4,092 / 183,635 /
4,088 / 256,374. Final pending work may be canceled on each joined stop; the last
active entry is reclaimed by quiescent cleanup. Scheduling-dependent Busy/frame
counts intentionally differ; compare invariants/reference audio, not those counts.
The one control and one audio thread overlap actual staging, publication, DSP and
reclamation. Test scheduler credits are capped; waits/yields/sleeps and fresh
reference Engine configuration occur outside production callback guards. Each
run enforces deadlines/log capacity and finishes without deadlock.

CSV `ownership.csv`/local receipt records observation order, event, full token,
generation, index, frames and control phase/token snapshots. Events: S staging,
P publication, A complete audio receipt, F generation-checked reclamation, X
rejected load, Q stop request, J joined cleanup. These are observations after API
calls, not an instruction-level trace or instantaneous global snapshot. All
adopted tokens are unique; every F token previously rendered; frame totals match.

Contract tests deliberately occupy all four states simultaneously; prove Busy
retains ready staging, full retirement defers switch/stop, stale handles reject,
settings/allocation failures leave active history/output intact, no-thread cleanup
needs no ISR, and restart/generation reuse works. Twenty-four forced C++ allocation
failures return AllocationFailed with no leaked staging. Utility and RAT rate tests
cover 8k/44.1k/96k/192k with exact equality and existing RAT restrictions. Extreme
finite, NaN/Inf input remains bounded and matches independently prepared fresh
Engines for the block/reset policy. Guard self-test detects deliberate allocation,
lock and IO attempts; actual callback counters remain zero.

During development, stress exposed consumption arriving between control loads;
publication now synchronizes bookkeeping **after** observing empty. Stop can retire
a just-consumed pending token between observations; generation ACK handles that
case without premature reuse. These races were repaired before final runs.

## Costs and measured host preparation versus latch

Host complete Runtime **45,288 B**, four bundles **45,216 B**, each Engine
**11,048 B**. ARM **45,224 / 45,152 / 11,040 B** respectively. Shared atomic words
are 16 bytes included in Runtime. ARM control/audio object text **4,949 / 712 B**,
0 data/BSS, excluding linked dependencies and the caller-allocated pool. Direct
static frames: stage 592 B, Engine::load 304 B, audio 72 B, not cumulative high-water.
Aligned uint32 atomics compile lock-free to loads/stores plus DMB, no exclusive
retry/helper. Audio undefined symbols are only RAT process and bounded memset.

Release contract trace counted **1,260 C++ allocations**, **73,558 cumulative
requested bytes** during its tracked control stages (not peak live RAM). Callback:
**zero allocation/free, lock and IO counts**. Peak parser/allocator/thread stack
use is unmeasured. Fixed test log buffers are 4,480,000 B, token map 16,384 B,
worker IO arrays 512 B and one fresh reference Engine (~11 KiB), all host-only.
Post-run CSV assembly and sanitizer overhead are additional test costs.

One observed Release stage sample per slot: utility 26.3 / 21.8 microseconds,
RAT composite 8.06 ms (first emplacement), reversed RAT 3.92 ms (reuse). Existing
RAT emplacement/configure may prepare twice on first construction; unchanged.
A separate 256-switch, **zero-frame boundary-only** sample measured 20 ns minimum,
30 ns median, 171 ns maximum including host timer/guard overhead. No DSP work in
that timing window. This is host observation, not ARM utilization, worst case,
full-block deadline, or proof of low switching latency/smoothness.

## Changed files, remaining gates

- `parable_activation.h`, control/audio `.cpp`: bounded pool, ownership handoff,
  lifecycle and structured errors. No public Engine/setter access.
- Activation tests/guard and demo: actual journal -> Codec -> prepare -> Engine
  path, exact output, receipts, concurrency/failure and resource evidence.
- `check_activation_arm.py`: reproducible compile-only audit against existing pins.
- CMake and CI: additive activation/demo/stress, ASan/UBSan, TSAN, host artifact and
  ARM inspection. Existing firmware build recipe/images are untouched.
- ADR addendum, protocol/evidence, README/status and patch-format cross-references:
  implemented host capability and limits, no correspondence or wire-format change.

Hardware remains **untested**: actual ISR lifecycle/quiescence, RAM placement and
heap/stack high-water, cache/MPU behavior, deadline/CPU, physical level/noise/latency
and switching transient. Atomic publication can click/reset tails; no crossfade,
state migration or musical approval. No persistent driver, live flash save,
transport or Volume/Register/modulation lane. All requested host acceptance criteria
pass locally; independent review/CI completion is recorded in the PR. One unmerged
PR; no physical compatibility claim. The living-modulation system is not begun.
