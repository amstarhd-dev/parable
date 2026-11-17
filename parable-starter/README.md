# Parable Core — Brick 0.1 (2026-11-06)

This is the first **portable C++ code brick**, not a finished Parable SDK and not a commercial DSP effect. It contains no astrology or browser implementation. A separate experimental circuit-derived LabRAT listening laboratory is described below.

SF-022 adds an experimental complete MIT [Leo grand hall](docs/SF022_REPORT.md)
and one coherent-reflection fault. [Download listening WAVs](samples/hall/README.md):
original/adopted healthy, damaged, stereo/mono, raw/RMS/wet and controlled probes.
The fixed474704B state and actual BOOT_NONE audio link fit the pinned memory map;
physical Seed3 timing/boot/audio and musical approval remain unproven. No Engine/
canonical/production firmware integration; Cancer PR untouched. [ADR022](docs/adr/ADR-022-leo-grand-hall.md).

SF-014 adds one standalone [loaded Lockhart wavefolder / upper-feed resistor fault](docs/SF014_REPORT.md).
CI publishes **parable-wavefolder-auditions** with the same Centaur/tape take03,
three drives, raw/common/RMS healthy/fault WAVs and same-input RAT/Muff/CMOS
comparisons. [ADR014](docs/adr/ADR-014-lockhart-feed-resistor.md) selects one cell,
512interval prepared curve and4x+ADAA. No Engine/firmware integration, measured
Seed3 throughput, physical transistor calibration or founder musical approval.

SF-011 adds an experimental standalone [original-style Centaur / one clean-wiper contact](docs/SF011_REPORT.md).
CI publishes **parable-klon-auditions**: three Gain pairs on new Guitar-TECHS take03,
raw/matched files, loaded path diagnostics, independent SPICE and source receipts.
[ADR-011](docs/adr/ADR-011-klon-clean-contact.md) records the selected S698 circuit,
100kΩ contact and12x accuracy/cost tradeoff. No Engine/patch integration,
physical supply/vintage-unit equivalence, Seed3 admission or founder approval.

SF-010 adds an experimental standalone [TS808 / one truly open feedback diode](docs/SF010_REPORT.md).
CI publishes **parable-ts808-auditions**: a new Guitar-TECHS take06, raw/matched
healthy/fault WAVs, controlled polarity/current/harmonic evidence and references.
[ADR-010](docs/adr/ADR-010-ts808-open-diode.md) records the documented signal-path
reduction, finite op-amp assumptions and16x accuracy/resource tradeoff. No patch/
Engine integration, physical vintage-unit matching or founder listening approval.

SF-009 adds a standalone [V3 sustain fuzz / R15 bias-starvation experiment](docs/SF009_REPORT.md).
CI publishes **parable-muff-auditions**, using a **new Guitar-TECHS DI performance**,
raw/matched guitar and controlled history probes. The actual fault primes the
following weak probe; intended post-overload suppression is **not established**.
[ADR-009](docs/adr/ADR-009-big-muff-bias.md) records the source and failed hypothesis.
No Engine/patch integration, measured EHX/Seed3 fidelity or founder approval.

SF-007 adds a standalone [two-rotor rotary / worn horn-belt experiment](docs/SF007_REPORT.md).
`parable_rotary_lab --input permitted-mono-pcm16.wav --output DIR` renders the
same guitar healthy/faulted, raw/matched and band taps, with actual motor/rotor
torque-motion CSV/SVG and synthetic probes. CI publishes **parable-rotary-auditions**.
[ADR-007](docs/adr/ADR-007-rotary-belt-model.md) records the reduced model and
limits. No Engine/canonical registration, physical motor fit or founder approval.

SF-006 adds a standalone [reduced tape echo / weak erase experiment](docs/SF006_REPORT.md).
Run `parable_tape_lab --input permitted-mono-pcm16.wav --output DIR`: healthy/fault
raw and RMS-matched audio, plus a feedback-zero A→different-B medium-memory proof.
CI publishes **parable-tape-auditions** with a pinned CC0 real-guitar recording,
WAVs/receipts/provenance. No Engine/patch integration, physical tape calibration,
SDRAM operation or founder musical approval is claimed. See [ADR-006](docs/adr/ADR-006-reduced-tape-medium.md).

PAR-009 prepares a [Seed3 first-day procedure](docs/SEED3_FIRST_DAY.md) and
[prior-art/firmware audit](docs/PAR009_GATE0.md), with three BOOT_NONE images:
standalone LED/recovery, mono bypass, and the unchanged RAT composite. The pinned
ARM command below produces `build/arm/first-boot/`; CI publishes
**seed3-first-boot-kit** for 30 days. It includes checksums, manifest, maps, static
frames, licenses and the original bypass rollback. **Nothing has been flashed or
physically tested.** The native Mac DFU candidate's deployment metadata is audited,
not an execution/USB guarantee for macOS 10.13.

Only the two new audio targets compile the DWT hooks. They capture and freeze the
first 30,000 callbacks, then publish once using a lock-free flag; USB CDC starts
after capture and repeats an immutable integer report. DSP continues. No callback
allocation/IO/locks/float reporting; the actual ARM accounting object has no
external helpers or floating instructions. The normal empty-patch bypass still
links generic Engine/RAT capability but configures/processes **no RAT**. Legacy
targets have no hooks and retain their binary hashes. The LED target has no
Engine/RAT/codec or external-memory initialization.

Timing spans callback-body entry through its final DWT timestamp/output barrier,
excluding bookkeeping/return/SDK ISR and later USB. Counter wrap requires intervals
<2^31 cycles; stopped load adds a millisecond wrap guard. RCC-derived core clock
and configured sample rate are not independent physical clock measurements.
Worst observed isn't worst possible or true p99. Postcapture CDC uses a short
control-side IRQ guard; the SDK USB class uses malloc/free. Its IRQ/heap/driver
robustness and guard latency remain unproven; they do not enter the captured
window. No claimed target utilization, latency/noise, heap/stack high-water or
guitar/musical approval. See the guide before connecting any carrier/power/audio.

PAR-008 adds one experimental RAT to the canonical serial patch engine, reusing
the unchanged LabRAT kernel. See [Gate 0](docs/adr/ADR-001-par008-gate0.md) and
[commands/evidence/limits](docs/PAR008_EVIDENCE.md). Run
`./build/parable_patch_audition --output build/patch-auditions` after a host build:
dry, RAT-only, Gain -> RAT -> Lowpass and reversed-order synthetic WAVs use actual
decode -> prepare -> Engine playback, with raw levels and no RMS matching.
Release CI publishes **parable-patch-rat-auditions**. Real-guitar/founder approval
and Daisy execution remain unproven.

SF-002 adds a standalone experimental CD4049 inverter laboratory through
Soundforge: [research/limitations/tests](docs/CMOS_REPORT.md),
[observed reuse](docs/SOUNDFORGE_SF002_WORK_LOG.md). It has no patch-runtime
integration or founder musical approval. Run `parable_cmos_lab --output DIR`
from the host build for synthetic listening WAVs; optional `--input` accepts
owned mono PCM16 through the existing reader. CI publishes the audition artifact.

SF-003 adds one positive-ground two-transistor germanium specimen with source
loading, leakage and physical feedback: [research/tests/limits](docs/GERMANIUM_REPORT.md)
and [measured reuse](docs/SOUNDFORGE_SF003_WORK_LOG.md). Run
`parable_germanium_lab --output DIR` for both RMS-matched and raw-relative synthetic
WAVs; optional owned mono PCM16 input uses the existing reader. Musical/physical
approval and patch integration remain separate gates.

PAR-010 adds a host-only [four-slot canonical preset journal](docs/PRESET_JOURNAL.md),
with [Gate 0 audit](docs/PAR010_GATE0.md) and [reproducible evidence](docs/PAR010_EVIDENCE.md).
It stages complete documents in independent A/B banks and tests interrupted writes
using deterministic simulated flash. It does not add a Seed3 driver, live switching
or physical persistence evidence. The existing native Bank remains RAM-only.

PAR-011 implements [host prepared-bundle activation](docs/PREPARED_ACTIVATION.md)
using the actual canonical Codec, prepare bridge and Engine. A fixed four-entry
pool separates active/pending/staging/retired ownership. Control prepares; audio
adopts once per block and acknowledges retirement. This is not clickless or
hardware-tested. [Evidence and commands](docs/PAR011_EVIDENCE.md) include concurrent
DSP/reference checks, sanitizers, memory and pinned ARM compile inspection.
After building, `./build/parable_activation_demo 0 2 1 3 0` selects saved fixture
presets by number and renders blocks with deterministic receipts. No live flash,
Volume/Register/modulation lane or firmware integration is added.

## Purpose

PAR-012 adds an experimental host [living-source lane](docs/LIVING_SOURCE_REQUIREMENTS.md)
around the unchanged prepared Runtime: heterogeneous source facts, deterministic
replay, consent/freshness and bounded test gain envelopes. Run
`./build/parable_living_demo --output build/living-evidence` for synthetic WAVs,
the same saved canonical RAT patch and timestamped explanation traces.
[Evidence/commands/limits](docs/PAR012_EVIDENCE.md); Release CI publishes
**parable-living-source-evidence** for 30 days. No wearable acquisition, Body
Language grammar, physiological mapping, firmware integration or listening
approval is claimed.

- A versioned, validated patch snapshot (`schema=2` for this native proof; **not** yet the final JSON/Wire format).
- Four independent in-memory performance slots.
- Fixed-capacity ordered stages and deterministic processing with no allocation in the audio callback.
- A separate target adapter for libDaisy firmware and host tests sharing the same portable Engine/RAT implementation (link `parable_rat`; the Engine is no longer header-only).
- Atomic acceptance/rejection of individual edits to a stored patch.

## Host tests (run without Daisy hardware)
From the repository root, with CMake >= 3.15, a C++17 compiler and Python >= 3.9:
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
bridge, host journal and host bundle activation are implemented; hardware activation and real flash storage remain deferred.

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
PAR-008 maps the additive `Rat` settings with a one-instance limit; utility-only
capacity stays eight. The Engine reserves one optional RAT state's storage even
for a utility patch. Load configures it only with processing stopped. This is a temporary boundary, not the final immutable plan or live/clickless
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
PAR-008 additionally links an explicitly active static Gain/RAT/Lowpass test patch
as `rat_integration/parable_seed3_rat_probe.elf`, retaining the bypass-startup
`minimal/` image. This build-only native fixture is not a save/transport contract.

## Not implemented / not validated
- **Daisy adapter is ARM-linked but has not been flashed or hardware-tested.**
- No persistent flash, power-loss safety, USB/Wi-Fi transfer, phone editor, footswitches, or Register.
- No dynamic graph, parallel routing, effect resource estimator, smoothing/clickless patch switching, DSP load metrics or hardware sample-rate query.
- LabRAT is experimental and now available as one canonical serial processor; no factory-fidelity/pedal-grade drive, compressor, delay, reverb, Moon, or astrology is proven.
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

## LabRAT circuit and malfunction auditions (PAR-007)

The host laboratory now uses [DSP Soundforge](tools/soundforge/README.md): shared
signal analysis, the existing PCM16 I/O, and an offline evidence/provenance
registry. See the [before/after baseline](docs/SOUNDFORGE_BASELINE.md) and next-model
recipe. This infrastructure does not add an effect, solver or patch integration;
LabRAT DSP and all sixteen original audition/report outputs are regression-checked.

[Source audit](docs/LABRAT_SOURCE_AUDIT.md) -> [proposed ADR-003](docs/adr/ADR-003-labrat-circuit-model.md)
-> [listening/measurement report](docs/LABRAT_REPORT.md). Eleven synthetic dry,
healthy low/medium/high, Filter, dynamics and component-variation WAVs use the
shared native RAT kernel. Founder musical approval remains pending.

From repository root:

```bash
cmake -S parable-starter -B parable-starter/build/rat-release -DCMAKE_BUILD_TYPE=Release
cmake --build parable-starter/build/rat-release -j2
ctest --test-dir parable-starter/build/rat-release --output-on-failure --no-tests=error
./parable-starter/build/rat-release/parable_rat_lab --output parable-starter/build/rat-auditions
```

Open the WAVs in a player/DAW, or download **parable-labrat-auditions** from the
passing Release Host tests workflow (seven-day retention). `--input dry-guitar.wav`
accepts bounded mono PCM16 without changing DSP code. No real-guitar or hardware
proof is claimed. PAR-008 adds the explicit patch-v1/Engine adapter; the PAR-006
processor is not substituted for the RAT. Historical factory revision, full
LM308/JFET dynamics and faulty-buffer headroom remain unverified.
