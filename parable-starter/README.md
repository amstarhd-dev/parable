# Parable Core — Brick 0.1 (2026-11-06)

This is the first **portable C++ code brick**, not a finished Parable SDK and not a commercial DSP effect. It intentionally contains no astrology, browser code, or expensive faux analog models.

## Purpose
- A versioned, validated patch snapshot (`schema=2` for this native proof; **not** yet the final JSON/Wire format).
- Four independent in-memory performance slots.
- Fixed-capacity ordered stages and deterministic processing with no allocation in the audio callback.
- A separate target adapter for libDaisy firmware and a host test harness sharing the **same header**.
- Atomic acceptance/rejection of individual edits to a stored patch.

## Host tests (run without Daisy hardware)
From the repository root, with CMake >= 3.15 and a C++17 compiler:
```bash
cd parable-starter
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release
ctest --test-dir build/Release --output-on-failure
```
Tests use always-active `CHECK` calls, including setup, and run with `NDEBUG` in
Release. GitHub Actions runs both Debug and Release builds from the
repository-root `.github/workflows/host-tests.yml`.

Audio safety policy: non-finite inputs produce zero without changing filter
history. Finite inputs, gain results and filter history saturate at +/-256
internally; the existing final output clamp remains +/-1. This numerical rail
preserves headroom for all eight gain-2 stages on ordinary +/-1 input and avoids
overflow on extreme floats. It can change extreme overload/attenuation behavior;
it is not analog modeling or a complete limiter. See the PAR-003 status update
for regression, recovery and normal-range reference-test evidence. Daisy execution,
CPU overhead, real guitar and analog clipping remain untested.

See [`docs/IMPLEMENTATION_STATUS.md`](docs/IMPLEMENTATION_STATUS.md) for the
source inventory, measured baseline and untested behavior, and
[`docs/adr/ADR-001-patch-authority.md`](docs/adr/ADR-001-patch-authority.md) for
the selected Option A decision. The data contract and temporary native preparation
bridge are implemented; live activation and durable storage remain deferred.

## JSON patch contract (PAR-002)

[`include/parable_patch.h`](include/parable_patch.h) provides `Codec::decode`
and `Codec::encode` for `parable.patch` version 1. See the normative
[`docs/PATCH_FORMAT_V1.md`](docs/PATCH_FORMAT_V1.md) and golden fixtures for
required fields, numeric rules, limits and rejection behavior. This format is
separate from the DSP prototype's internal `schema=2`. Codec calls allocate and
belong outside the audio callback; the separate host bridge below does no parsing.

Host configuration fetches a SHA-256-pinned nlohmann/json 3.11.3 release. Its MIT
license, rationale and offline-build override are recorded in
[`third_party/README.md`](third_party/README.md). Debug and Release run all four
tests, including `patch_contract` and `host_preparation`, with checks active in both configurations.

## Native host preparation (PAR-004)

[`include/parable_prepare.h`](include/parable_prepare.h) provides
`parable::preparation::prepare(document, actualSampleRate)`. It resolves stable
IDs in explicit serial order and returns an owned native `Patch` or an Error
with code, typed field, rule, instance ID and native-validation status. The rate
must exactly match the document declaration; there is no automatic conversion.

Decode and prepare on the control side. Only after success, and **with audio
stopped**, call `Engine::load` and check its status. The Engine must use that same
actual sample rate. The native prototype patch carries neither rate nor IDs;
retain the validated document as authority. No Engine is changed by preparation.
The bridge library does not link JSON; host end-to-end tests explicitly link both.
This is a temporary boundary, not the final immutable plan or live/clickless
activation. PAR-005 cross-compiles preparation to an ARM object and links the
minimal native firmware; runtime CPU/RAM, hardware and musical audition remain unproven.

## ARM firmware build (PAR-005)

From repository root on Linux x86_64 with Python >=3.12, Git, GNU Make and curl:

```bash
python3 parable-starter/firmware/build.py
```

The script fetches verified/pinned sources and compiler, then writes ELF/bin/map,
logs, resource reports and notices under `parable-starter/build/arm/`. It never
flashes hardware. See [`docs/ARM_BUILD_BASELINE.md`](docs/ARM_BUILD_BASELINE.md)
for exact boot/linker selection, measured sections, retained warnings and bring-up
requirements. The recipe links minimal audio in internal flash and fuller startup
JSON/preparation diagnostics using the official BOOT_SRAM and BOOT_QSPI layouts.
The same diagnostic exceeds the 128 KiB BOOT_NONE limit: this is a layout limit,
not Daisy's total capacity. Seed3 bootloader/hardware operation remains unverified.
The planned ESP32-S3 could own JSON/control work; no controller or transport is
implemented here, and canonical JSON functionality remains unchanged.

## Not implemented / not validated
- **Daisy adapter is ARM-linked but has not been flashed or hardware-tested.**
- No persistent flash, power-loss safety, USB/Wi-Fi transfer, phone editor, footswitches, or Register.
- No dynamic graph, parallel routing, effect resource estimator, smoothing/clickless patch switching, DSP load metrics or hardware sample-rate query.
- No modeled overdrive, compressor, delay, reverb, Moon, or astrology.
- The one-pole filter and gain nodes are **plumbing**, not an effect catalog.
- `Engine::load` must only be called while the audio callback is stopped; cross-thread patch exchange is not yet implemented. The prototype uses a mono signal and copies it to both outputs.
- `schema=2` is a provisional code-internal tag; does not yet specify durable wire compatibility.

## Next build steps
1. Acquire Seed3 **65MB** module and, ideally, official pedal dev kit; review the pinned build/boot choice, flash only in a separate bring-up task, and measure pass-through and noise/latency.
2. Build double-buffered/atomic patch activation and 4-slot durable journal with CRC and power-loss tests.
3. Review the version-1 contract/temporary native bridge and measure target feasibility before live activation; binary interoperability remains deferred.
4. Add realistic bypass and safe level handling, measure callback CPU/memory; then one carefully modeled drive family at a time.
5. Implement separate communication MCU (if selected) and web editor against the frozen patch contract.

Design rule: one patch language, many composers. Astro, manual workshop, future biometric or other symbolic idioms compile to the same validated graph.

## Codex onboarding

For the first cloud task, see [`docs/CODEX_KICKOFF.md`](docs/CODEX_KICKOFF.md). This is an engineering baseline, **not** complete product firmware.
