# PAR-008 integration receipts

Base main `440a99221af7cc3f3b7123db327bb161306d4856`. Required reading and
[Gate 0](adr/ADR-001-par008-gate0.md) preceded source edits. No framework,
RAT equations/components/faults, astrology, live activation or storage added.
Earlier reports describe their historical baselines; this receipt supersedes
their statement that RAT is not registered/firmware-linked. It does not supersede
their scientific, physical-fidelity or musical limitations.

## Reproduction and test boundary

From repository root, C++17/GCC, CMake >=3.15, Python >=3.12 for ARM, Git/Make/curl:

```sh
cmake -S parable-starter -B build/par008-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/par008-Debug -j2
ctest --test-dir build/par008-Debug --output-on-failure --no-tests=error
cmake -S parable-starter -B build/par008-Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/par008-Release -j2
ctest --test-dir build/par008-Release --output-on-failure --no-tests=error
./build/par008-Release/parable_patch_audition --output build/par008-auditions
python3 parable-starter/firmware/build.py
```

Local Linux x86_64 GCC 14.2.0/CMake 3.31.6 baseline: **16/16 Debug and Release**.
Integration adds `rat_patch_integration` and `patch_audio_evidence`: **18/18** in
each configuration. Tests check independent native RAT/serial-chain results,
golden bytes, stable identity/order, all control endpoints individually at
32/48/96 kHz, disabled-state bypass, one-RAT/rate/errors, reset/preset reload,
failure atomicity with evolving histories, NaN/Inf/extreme bit patterns, finite
bounds and zero observed processing allocations. Old tests/fixtures are retained.

Sanitizer reproduction (same flags and targets as local and CI):

```sh
cmake -S parable-starter -B build/par008-sanitize -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined -no-pie"
cmake --build build/par008-sanitize --target parable_core_test parable_audio_safety_test parable_patch_test parable_prepare_test parable_rat_test parable_rat_integration_test parable_patch_audition -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/par008-sanitize --output-on-failure --no-tests=error -R '^(core_validation|audio_safety|patch_contract|host_preparation|rat_patch_integration|patch_audio_evidence|rat_circuit_and_safety)$'
```

**7/7 ASan/UBSan** affected-boundary tests; not a sanitizer pass for the entire
CMOS/germanium labs. Full ordinary suites include those independent science/CLI
regressions. RAT's original 16 files and protected sources still match its
committed regression. Local before/after CMOS **13/13 files** and germanium
**25/25 files** match byte-for-byte. ARM RAT/CMOS/germanium standalone object bytes
also match baseline. This is regression evidence, not new physical validation.
Exact final CI runs/head/artifact receipts and actual local paths are in the PR.

## Host audio evidence

Four owned synthetic 9 s / 48 kHz mono PCM16 WAVs run through canonical decode ->
prepare -> stopped Engine. Each artifact includes its canonical `.patch.json`,
`renders.csv`, `checks.txt`, source/compiler identity, SHA-256 and borrowed notices.
No RMS matching or file gain (post gain=1); master Volume=1. Dry trim=1; others=.8.
RAT nominal controls .35/.5/.7, 4x; Gain=1.8, Lowpass=1800 Hz. The software scale
is 1 engine unit/model input V, bounded +/-4 V at RAT, existing jack V/4 output.

| Render | Float peak | RMS | DC mean |
| --- | ---: | ---: | ---: |
| Raw/dry | .150000006 | .0133712411 | .0000175143 |
| RAT only | .0888038427 | .0295070478 | .0000010663 |
| Gain -> RAT -> Lowpass | .0880397707 | .0346532708 | .0000011060 |
| Lowpass -> RAT -> Gain | .162864253 | .0528105325 | .0000024464 |

RAT-only vs standalone peak absolute error **0** with documented scaling/trim.
Forward vs reverse RMS difference **.0204020488** without delay/gain fitting:
pre-gain changes excitation into clipping; pre-filter changes its spectrum.
Pretrim stats reconstruct output/trim only after checking the final clamp did
not engage. Values precede undithered PCM16 quantization. No tail extension or
delay compensation; no real guitar, founder listening or factory-pedal approval.
Download `parable-patch-rat-auditions` from passing Release CI (seven-day retention).

## Actual pinned ARM link

Unchanged xPack GCC 14.2.1-1.1, libDaisy `1289a000bc29dae1bc0ed4129cfa2df40ef71574`,
driver/archive pins and upstream linker scripts. Cortex-M7/FPv5-D16 hard float,
O2 application, IEEE finite checks. All four successful images are real ARM ELFs.

| Image | Berkeley text/data/BSS bytes | Load/bin span | Execution |
| --- | --- | ---: | --- |
| Main baseline bypass | 72,392 / 1,616 / 28,608 | 74,008 | internal FLASH |
| Current bypass startup (rollback) | 93,524 / 1,696 / 39,784 | 95,220 | internal FLASH |
| **Active Gain/RAT/Lowpass** | **93,660 / 1,696 / 39,784** | **95,356** | **internal FLASH, BOOT_NONE** |
| Startup codec SRAM diagnostic | 217,576 / 2,092 / 39,856 | 219,720 | AXI SRAM, BOOT_SRAM |
| Startup codec QSPI diagnostic | 217,576 / 2,092 / 39,856 | 219,720 | external QSPI XIP, BOOT_QSPI |

Active image: vector 664 B at `0x08000000`, `.text` 93,652 B (includes rodata) at
`0x08000298`, exidx 8 B, init/fini arrays 40/4 B. `.data` 988 B at `0x24000000`
(flash initializer at `0x080170a0`); `.bss` 23,388 B at `0x240003e0`.
AXI static section sum/span **24,376/24,380 B**, D2 DMA **16,384 B**, backup **12 B**;
no static external SDRAM. Stack top `0x20020000` in DTCM, high-water unmeasured.
Flash load headroom **35,716 B**; active image adds **21,348 B** over main baseline.

ARM Engine **11,040 B**, native Patch **204 B**; static optional Engine allocation
**11,048 B** vs baseline **184 B**. One RAT instance **10,752 B**; standalone object
**6,452/0/0 B**, not confused with linked cost or working RAM. Utility-only Engine
reserves the same optional storage and processing code, while RAT construction
occurs only on a stopped RAT load. All eight utility stages/four patch slots remain.

Compiler direct frames in the active build: callback **48**, Engine load **296**,
main **216**, RAT process **40**, circuitSample **24**, clipping process **48**,
configure **424 B**. These are `.su` frames, not cumulative IRQ/call-chain bounds
or measured stack. Final images have **zero unresolved symbols**; disassembly
check requires a callback/Engine call to actual `Rat::process`. RAT object checks
retain allocation/throw/IO-symbol rejection. Parser symbols are absent in active
and bypass images; JSON exists only in startup codec diagnostics.

The full-codec BOOT_NONE diagnostic still fails its expected layout gate:
**88,800 B overflow**, versus baseline **65,380 B**. SRAM/QSPI alternatives link;
the active RAT image itself needs neither. No FLASH region enlargement or library
functionality deletion. Original firmware binaries intentionally change because
their common Engine now includes RAT; rollback is bypass-startup behavior, not
old binary identity. SDK FP-interrupt, nosys unimplemented-syscall and RWX-segment
warnings persist unchanged in kind; maps/logs retain them. No unexpected link/API
failure was accepted. Baseline ARM input hashes equal main; its dirty flag records
only the newly written Gate 0 note. Final CI identifies its actual checkout.

## Limits and next smallest gate

Software integration is demonstrated; **Daisy execution/performance is unmeasured**.
No flashing, codec/electrical calibration, guitar loading, boot compatibility,
measured clocks/latency/noise, callback deadlines, DWT timing, power, stack/heap
high-water, musical approval, persistent presets or live/clickless exchange.
Source consumers of Engine must now link `parable_rat`; native structs are not a
binary save/transport ABI. Old readers reject Rat; future controller/editor
capability negotiation and revision migration remain separate design questions.

Next ticket: separately authorized Seed3 bring-up/resource gate under #24. Verify
module/boot/recovery and analog interface, establish bypass first, then activate
this exact chain; measure worst-case callback/IRQ/stack budget and overload/reset
behavior, levels/noise/latency, then obtain real-guitar/founder listening evidence.
Keep bypass rollback. Do not add another processor before that target evidence.
