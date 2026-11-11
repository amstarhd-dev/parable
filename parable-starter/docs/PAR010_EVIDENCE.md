# PAR-010 reproducible host evidence

Base main `156fa08bb06a2018270eda0a6cc44b09bb8ff869` includes merged PR #30.
[Gate 0](PAR010_GATE0.md) was recorded before source edits. No blocker required
hardware geometry selection or a memory-map refactor. GCC 14.2.0, CMake 3.31.6,
Python 3.12.14, Linux x86-64; actual host tests, no performance extrapolation.

## Reproduce from repository root

CMake fetches the existing SHA-pinned nlohmann/json 3.11.3 archive. No new fetched
package, filesystem or flash manager. Standard C++17/Python/CMake prerequisites
are the same as main. Local builds used these exact commands (CMake on PATH):

```bash
cmake -S parable-starter -B /workspace/work/par010-before-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/par010-before-Debug -j2
ctest --test-dir /workspace/work/par010-before-Debug --output-on-failure --no-tests=error
cmake -S parable-starter -B /workspace/work/par010-before-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/par010-before-Release -j2
ctest --test-dir /workspace/work/par010-before-Release --output-on-failure --no-tests=error
```

Those baseline commands ran **before edits**. For the implementation:

```bash
cmake -S parable-starter -B /workspace/work/par010-after-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/par010-after-Debug -j2
ctest --test-dir /workspace/work/par010-after-Debug --output-on-failure --no-tests=error
cmake -S parable-starter -B /workspace/work/par010-after-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/par010-after-Release -j2
ctest --test-dir /workspace/work/par010-after-Release --output-on-failure --no-tests=error
cmake -S parable-starter -B /workspace/work/par010-sanitize -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build /workspace/work/par010-sanitize --target parable_core_test \
  parable_audio_safety_test parable_patch_test parable_prepare_test parable_rat_test \
  parable_rat_integration_test parable_patch_audition parable_firmware_measurement_test \
  parable_presets_test -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir /workspace/work/par010-sanitize --output-on-failure --no-tests=error \
  -R '^(firmware_measurement_math_status|core_validation|audio_safety|patch_contract|host_preparation|rat_patch_integration|patch_audio_evidence|rat_circuit_and_safety|preset_journal)$'
/workspace/work/par010-after-Release/parable_presets_test > /workspace/work/par010-fault-matrix.txt
/workspace/work/par010-after-Release/parable_presets_test > /workspace/work/par010-fault-repeat.txt
cmp /workspace/work/par010-fault-matrix.txt /workspace/work/par010-fault-repeat.txt
python3 parable-starter/tests/fixtures/presets/generate.py
git diff --exit-code -- parable-starter/tests/fixtures/presets/slot2-gen9.header.hex
git diff --check
```

For another checkout replace `/workspace/work/par010-*` with local build/output
paths. GitHub host CI runs the same full suites and adds the new journal to the
existing sanitizer job. Release also repeats/compares its fault matrix and publishes
`parable-preset-journal-evidence` for 30 days with compiler/source/checksums.

## Results and fault matrix

- Baseline: **19/19 Debug**, 80.16 s; **19/19 Release**, 20.14 s.
- Final: **20/20 Debug**, **20/20 Release**, targeted **9/9 ASan/UBSan**.
- Two deterministic Release sweeps produce identical output (cmp exit 0).
- Existing patch goldens and DSP/kernel files are unchanged; full RAT/CMOS/germanium
  circuit, safety, lab/CLI, output-preservation and patch playback regressions pass.
- No new compiler warnings in the final host build. An initial development test
  referenced a nonexistent cycle fixture; corrected to the existing fixture before
  final runs. No failing acceptance criterion was waived.

| Scenario | Program / erase bytes | Byte cuts | Old/default | Fully committed new | I/O failures |
| --- | --- | --- | --- | --- | --- |
| First save, generation 0→1 | 8 / 64 | 16,921 | 16,920 | 1 | 511 |
| Overwrite, 2→3 | 8 / 64 | 16,921 | 16,920 | 1 | 383 |
| Clear, 2→3 | 8 / 64 | 16,585 | 16,584 | 1 | 340 |
| Large canonical, 2→3 | 256 / 4096 | 45 | 44 | 1 | 109 |
| Alternate geometry | 1 / 256 | 595 | 594 | 1 | 600 |
| Alternate geometry | 16 / 256 | 276 | 275 | 1 | 229 |
| Alternate geometry | 64 / 256 | 221 | 220 | 1 | 211 |

Total **51,564 byte cuts**, **2,383 I/O-boundary failures**, plus **512 seeded
ongoing reset/save cycles**. Digest `1b66b2d9a969bef5` is a reproducibility checksum,
not a cryptographic proof. Small-profile first-save/overwrite/clear cut every
mutating byte, including the final marker byte. Other geometries cover operation
boundaries and representative interior bytes. Every injected fault must avoid a
`Saved` acknowledgment; reboot must return an explicit default, old valid record
or complete new record. Final-byte cuts and lost final-read acknowledgments can
recover the new committed record, as documented by `CommitUncertain`.

Additional checks cover four distinct utility/RAT documents and stable ID/routing
bytes; physical isolation of other slots and outside partition sentinels; separate
clear; independently Python-packed/zlib envelope; invalid JSON/units/routes/revision;
CRC/header/length/slot/generation corruption; invalid/noncanonical committed payloads;
future envelope/patch/type/revision (including added fields), conflicts, generation
exhaustion, too-small/invalid geometry, 0→1/alignment violations, read failures,
wear budget retained over resets, and backend-returned-Ok corruption detected by
erase/payload/commit readback.

The maximum-count, high-ID fixture has one RAT and seven lowpasses, **1,653 canonical
bytes**. A valid **16,384-byte input** padded with whitespace saves/reloads those
exact canonical bytes; 16,385 rejects without mutation. Current closed eight-stage
v1 cannot express an arbitrary 16 KiB canonical payload; no unsupported padding
fields or schema changes were invented. Full 16 KiB capacity is reserved per bank.
In the 4096/256 profile this tested record programs 2,304 bytes (256 header area,
1,792 padded payload, 256 commit) and erases its 20,480-byte inactive bank.
The full journal reserves **163,840 bytes**. See protocol for allocation bounds.

## Changed files and limits

- `include/parable_presets.h`, `src/parable_presets.cpp`: bounded control-side
  journal/API, complete canonical records, recovery and explicit clear.
- `include/parable_flash_sim.h`, `src/parable_flash_sim.cpp`: deterministic host
  bytes, geometry, power cuts, corruption, tracing and wear faults.
- `src/preset_crc.h`, `third_party/crc32/`: narrow MIT CRC reuse/provenance.
- `tests/test_presets.cpp`, `tests/fixtures/presets/`: fault matrix and independent
  Python/zlib envelope oracle; no changes to old golden fixtures.
- CMake/host workflow: new separate targets, additive sanitizer coverage, repeated
  evidence artifact; existing tests/firmware workflow retained.
- Gate 0/protocol/evidence, README/status, patch-format/ADR cross-references and
  third-party README: actual state and boundaries, not new patch semantics.

**Not tested/implemented:** real flash driver, physical persistence/endurance/power
cuts, boot/runtime hardware, MCU RAM/stack high-water, callback CPU/latency, live
preset switching, transport/UI or musical audition. Optional ARM compilation of
this new control-side module was not performed locally; it is not linked into
firmware. The unchanged ARM CI still guards existing firmware builds, not this
journal. Simulated prefix faults/bit flips cannot prove real QSPI behavior.
Hardware geometry/atomicity, loader-reserved partition, wear policy, execute/write
restrictions and audio-pause requirements remain explicit future adapter gates.
No blockers or intentionally omitted host acceptance criteria. PR remains for
independent review; do not merge as part of this task.

Next smallest ticket: host-simulated atomic runtime four-preset bundle activation
using ADR-001 active/pending/retired ownership, separate from physical storage.
