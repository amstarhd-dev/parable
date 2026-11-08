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
for regression, recovery and normal-range reference-test evidence. Daisy firmware,
CPU overhead, real guitar and analog clipping remain untested.

See [`docs/IMPLEMENTATION_STATUS.md`](docs/IMPLEMENTATION_STATUS.md) for the
source inventory, measured baseline and untested behavior, and
[`docs/adr/ADR-001-patch-authority.md`](docs/adr/ADR-001-patch-authority.md) for
the proposed patch decision. The architecture proposal is not implemented.

## Not implemented / not validated
- **Daisy adapter has not been compiled or flashed** (no libDaisy toolchain connected here).
- No persistent flash, power-loss safety, USB/Wi-Fi transfer, phone editor, footswitches, or Register.
- No dynamic graph, parallel routing, effect resource estimator, smoothing/clickless patch switching, DSP load metrics or hardware sample-rate query.
- No modeled overdrive, compressor, delay, reverb, Moon, or astrology.
- The one-pole filter and gain nodes are **plumbing**, not an effect catalog.
- `Engine::load` must only be called while the audio callback is stopped; cross-thread patch exchange is not yet implemented. The prototype uses a mono signal and copies it to both outputs.
- `schema=2` is a provisional code-internal tag; does not yet specify durable wire compatibility.

## Next build steps
1. Acquire Seed3 **65MB** module and, ideally, official pedal dev kit; compile and flash the adapter; measure pass-through and noise/latency.
2. Build double-buffered/atomic patch activation and 4-slot durable journal with CRC and power-loss tests.
3. Define a canonical cross-platform binary/JSON patch serialization and graph validation contract.
4. Add realistic bypass and safe level handling, measure callback CPU/memory; then one carefully modeled drive family at a time.
5. Implement separate communication MCU (if selected) and web editor against the frozen patch contract.

Design rule: one patch language, many composers. Astro, manual workshop, future biometric or other symbolic idioms compile to the same validated graph.

## Codex onboarding

For the first cloud task, see [`docs/CODEX_KICKOFF.md`](docs/CODEX_KICKOFF.md). This is an engineering baseline, **not** complete product firmware.
