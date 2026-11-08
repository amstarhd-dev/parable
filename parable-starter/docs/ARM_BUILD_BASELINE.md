# PAR-005 ARM build/resource baseline

This gate proves cross-compilation/linking, not running hardware. Started from
main `849143b0886eb9de0cc6eceba7d5d9c6345df53c` after PR #7 merged. The unchanged
host Debug and Release suites each passed 4/4 before firmware changes.

## Clean-checkout reproduction

Host requirements: Linux x86_64, Python >=3.12, Git, GNU Make and curl. No root,
system toolchain installation, Docker, connected board or flashing tool required.
From the repository root:

```bash
python3 parable-starter/firmware/build.py
```

Default output is `parable-starter/build/arm/`; managed dependency cache is
`parable-starter/build/arm-cache/`. Explicit paths are supported:

```bash
python3 parable-starter/firmware/build.py --cache /workspace/work/par005-fresh-cache --output /workspace/work/par005-three-layout-build
```

The script verifies SHA-256 archives, fetches exact commits and required driver
gitlinks, rejects modified/mismatched cached sources, rebuilds libDaisy, links
minimal ELF/hex/bin, cross-compiles preparation, links full-codec diagnostics with
the upstream BOOT_SRAM and BOOT_QSPI layouts, and attempts the same diagnostic
in internal flash. Only the identified internal FLASH overflow is an accepted diagnostic
failure; an unexpected compile/API/undefined-reference failure fails the command.
It never runs ARM instructions or invokes DFU/OpenOCD. A `.bin` is not flashing proof.

All exact pins are in [firmware/dependencies.json](../firmware/dependencies.json);
licenses/rationale are in [ARM_DEPENDENCIES.md](../third_party/ARM_DEPENDENCIES.md).
The audited upstream was libDaisy commit `1289a000bc29dae1bc0ed4129cfa2df40ef71574`.
The chosen compiler is **xPack GNU Arm Embedded GCC 14.2.1-1.1**, compiler 14.2.1
(20241119), archive SHA-256
`ed8c7d207a85d00da22b90cf80ab3b0b2c7600509afadf6b7149644e9d4790a6`.
Arm's own archive endpoint was blocked here (403); xPack is identified explicitly.

Application flags: Cortex-M7 Thumb, FPv5-D16 hard float, GNU C++17, `-O2`,
`-DNDEBUG`, `-fshort-enums`, no RTTI/exceptions in the minimal app, function/data
sections and linker GC. libDaisy uses its default `-O3` plus `-fstack-usage`.
The pinned toolchain's SDK/application enum attributes agree (`small`). No unsafe
finite/fast-math assumptions: the app explicitly disables them and rejects such
flags at compile time. JSON diagnostic units enable exceptions on the control side.

## Seed3, codec and rate audit

Official [Seed3 specification](https://docs.daisy.audio/hardware/Seed3/) advertises
Cortex-M7/480 MHz, up to 192 kHz/32-bit audio, 65 MB RAM and 8 MB flash. The
[datasheet](https://daisy.nyc3.cdn.digitaloceanspaces.com/products/seed3/Daisy_Seed3_datasheet.pdf)
audited here is v2.1.0, SHA-256
`c4c13c15713f8c32432917afb79a75c09c0d4f69d9cb48ee02cadcfb82fb2ede`.
This baseline targets the **65 MB module**, not an assumed SDRAM-less variant.

At the pinned libDaisy source:

- `src/daisy_seed.h`: `DAISY_SEED_3`, documented TAC5242 codec.
- `src/daisy_seed.cpp::CheckBoardVersion`: PH6-based Seed3 detection.
- `ConfigureAudio`: Seed3 follows the common Seed transmit/receive pin path;
  reset-only `Ak4556` helper is also used for the strapped Seed3 codec. It does
  not configure every advertised TAC5242 capability through a new driver.
- Selected SDK configuration is **48 kHz, 24-bit SAI**, not a demonstrated
  192 kHz/32-bit path. `AudioHandle::AudioCallback` is the noninterleaved
  `InputBuffer, OutputBuffer, size_t` API used by this adapter.
- `DaisySeed::AudioSampleRate()` queries the configured AudioHandle rate.
  Firmware constructs its fixed-storage Engine from that query **after Init**,
  replacing the old independent hardcoded engine rate. It checks `load` before
  starting audio. This is software configuration consistency, not measured codec
  clock/rate correctness. `Init(true)` requests 480 MHz; block size is fixed to 48.
- Callback processes only input channel 0 and copies it to both outputs. It uses
  the unchanged bounded native core; no JSON, I/O, allocation or locks were added
  to that callback. Initialization and optional diagnostic decoding happen before
  `StartAudio`. No patch selection/live activation was added.

No board, codec reset, PLL, DMA or analog signal has actually been observed.

## Exact linker/boot choice and measured sizes

**APP_TYPE=BOOT_NONE**, `core/STM32H750IB_flash.lds`, STM32H750 macros. Internal
FLASH origin `0x08000000`, length **128 KiB**. No Daisy software bootloader image
is deployed; intended future loading is via STM32 ROM DFU or an approved
debug probe. SRAM/QSPI diagnostics are evaluated below. Flashing this layout would
replace software at the internal boot address; review any existing Daisy loader
and restoration plan before physical bring-up. No flashing was performed here.

The script records the linker file SHA-256:
`d45452b0f9f5b722b16e3a8fc168a76e2a5022d46a4efd10054880dcc5a87e8d`.
Its memory map also declares ITCM 64 KiB, DTCM 128 KiB, AXI SRAM 512 KiB,
D2 SRAM 288 KiB, D3 SRAM 64 KiB, backup SRAM 4 KiB, external SDRAM 64 MiB and
external QSPI 8 MiB. These are linker regions, not interchangeable free memory
or proof those memories operate. The advertised 8 MB flash is not this 128 KiB
internal image budget. The diagnostics below test external image placement;
physical boot and execution remain a separate gate.

Measured release minimal image (bytes):

| Measurement | Bytes / location |
| --- | --- |
| `size` Berkeley text / data / BSS | **72,392 / 1,616 / 28,608** |
| FLASH load span / `.bin` length | **74,008** at `0x08000000` |
| Output `.text`, includes read-only constants | **72,384** |
| Included input `.rodata` subtotal within `.text` | **512**; other literal pools/alignment are not separately classified |
| Vector / exidx / init / fini sections | **664 / 8 / 40 / 4** in FLASH |
| Initialized `.data` | **908**, VMA `0x24000000`, initialization bytes also in FLASH |
| Ordinary `.bss` | **12,212**, AXI SRAM |
| AXI static section sum / address span | **13,120 / 13,124** (4-byte alignment gap) |
| Audio DMA `.sram1_bss` | **16,384** at `0x30000000`, D2 SRAM |
| Backup SRAM `.backup_sram` | **12** at `0x38800000` |
| Static ITCM / DTCM / D3 / SDRAM / QSPI allocation | **0** each |
| Explicit `.heap` / `.reserved_for_stack` sections | **0 / 0**, not zero runtime heap/stack use |
| Initial main stack pointer | `0x20020000`, top of DTCM region |
| Compiler `.su` direct frames: AudioCallback / main | **20 / 120**, static bytes; not total call-chain/IRQ stack |
| Preparation-only ARM object `size` text/data/BSS | **8,380 / 0 / 0**; not linked/deployed in minimal image |

Berkeley `data` includes vector/init arrays in FLASH; it is **not** the initialized
SRAM total. BSS includes D2/backup regions. The upstream script folds `.rodata`
into `.text`; the report counts map input `.rodata` without mislabeling it as a
separate output section. Per-section VMA/LMA, map and compiler frames are retained.
Runtime stack high-water, interrupt nesting, heap use and exhaustion behavior are
**not measured**. Compiler frame numbers cannot be summed into a safe stack budget.
These minimal-image numbers exclude the preparation object/full JSON stack,
future drive/delay/reverb algorithms and buffers, USB UI/Wi-Fi/BLE or other patch
transport, persistent preset journals and live activation. They are not a pilot
effect-board resource budget.

Minimal `.bin` SHA-256, reproduced across two dependency paths:
`ddd86502b4a0b79c3bdb86afcc08d36a92947deb18e121865758036d19c634a5`.
ELF debug paths vary; byte-identical ELF output is not claimed. Reports include
Parable commit/dirty state and per-input hashes, compiler, source pins and linker
hash so local pre-commit measurements and clean CI evidence remain distinguishable.

### JSON/preparation feasibility boundary

Portable native core is **ARM-linked** in the minimal image. Native preparation
is **ARM cross-compiled to an object** and linked into both fuller diagnostic
images, not executed. Both JSON codec
translation units also compile with this toolchain (floating `charconv` is
available); that is not sufficient to claim a usable JSON-equipped firmware.

The release diagnostic additionally attempts startup-only decode -> prepare ->
load using the same audio adapter, with no callback parsing. Its link fails:
**FLASH overflow by 65,380 bytes**, requiring **196,452 bytes** in the chosen
131,072-byte internal region. Make exits 2, linker exits 1. Its map/error log are
retained; no successful **internal-flash diagnostic** ELF/bin is advertised. Earlier exploratory
assertion-enabled builds were larger/path-dependent; the pinned baseline uses
explicit release `NDEBUG` and the numbers above.

This is a measured **BOOT_NONE layout limit**, not a general Daisy capacity
failure or a host-test failure. The very same decode/prepare diagnostic links in
both official larger layouts below. No functionality was removed to force it
into 128 KiB, and no linker script or internal FLASH capacity was enlarged.

### Official SRAM and QSPI layouts: actual builds

The recipe builds all three successful images, retaining a separate ELF/bin/map,
`size`, section VMA/LMA, symbols, disassembly and `.su` report per image. The fuller
diagnostics compile the existing JSON codec, parser and preparation sources and
call decode -> validate -> prepare -> stopped Engine load before `StartAudio`.
Linker garbage collection retains reachable code, not every library API. These
are startup bypass-document diagnostics, not target round-trip/preset tests or
complete product images. Parser, validation and correspondence semantics remain
unchanged; no new transport/serialization format is introduced.

| Actual measurement (bytes) | Minimal BOOT_NONE | Fuller BOOT_SRAM | Fuller BOOT_QSPI |
| --- | ---: | ---: | ---: |
| Berkeley text / data / BSS | 72,392 / 1,616 / 28,608 | 194,148 / 2,092 / 28,992 | 194,148 / 2,092 / 28,992 |
| Image load span / bin length | 74,008 | 196,292 | 196,292 |
| Output `.text` including constants | 72,384 | 189,156 | 189,156 |
| Included input `.rodata` within `.text` | 512 | 33,705 | 33,705 |
| Vector / extab total / exidx | 664 / 0 / 8 | 664 / 4,056 / 936 | 664 / 4,056 / 936 |
| Init / fini arrays | 40 / 4 | 44 / 4 | 44 / 4 |
| `.data` / ordinary `.bss` | 908 / 12,212 | 1,380 / 12,596 | 1,380 / 12,596 |
| AXI SRAM VMA section sum / span | 13,120 / 13,124 | 194,860 / 194,912 | 13,976 / 13,980 |
| DTCM static VMA section sum / span | 0 / 0 | 13,976 / 13,980 | 0 / 0 |
| D2 DMA / backup SRAM | 16,384 / 12 | 16,384 / 12 | 16,384 / 12 |
| On-chip RAM allocated VMA section bytes, including SRAM-executed code | 29,516 | 225,232 | 30,372 |
| External QSPI VMA section sum / span | 0 / 0 | 0 / 0 | 194,860 / 194,912 |
| Direct callback / main frame | 20 / 120 | 20 / 328 | 20 / 328 |

All three allocate zero static ITCM, D3 and external SDRAM. Explicit heap/stack
reservation sections remain zero, **not** zero runtime usage. All initial stacks
start at DTCM top `0x20020000`. Section sums omit alignment gaps; spans run from
first to last nonempty allocated VMA. Neither is an available-RAM measurement.

| Layout | Execution/vector and read-only sections | `.data` VMA / LMA | `.bss` VMA | Nonvolatile app storage |
| --- | --- | --- | --- | --- |
| BOOT_NONE | FLASH vector `0x08000000`, text `0x08000298` | `0x24000000` / FLASH | `0x24000390` | Internal FLASH, 128 KiB budget |
| BOOT_SRAM | AXI vector `0x24000000`, text `0x240002a0` | `0x20000000` / `0x2402f960` | `0x20000568` | Loader stores bin in QSPI at `0x90040000`, then copies to AXI SRAM |
| BOOT_QSPI | QSPI vector `0x90040000`, text `0x900402a0` | `0x24000000` / `0x9006f960` | `0x24000568` | QSPI at `0x90040000`, execute in place (XIP) |

All three binary hashes reproduced across two cache paths. Fuller diagnostic
SHA-256: SRAM `75548644fdca8dff8211a91db278cb550277268b16e7a6e7dc23aa4958532d86`;
QSPI `fbef6f14be4a20ac31379451159a2015f12480725b91aaac91c4270d3092e1b8`.
The script also checks ARM attributes, selected load range, vector/MSP/Thumb-reset
placement, bin length, codec/preparation symbol retention in diagnostics and
codec exclusion from minimal. These checks inspect files; they execute no firmware.

For BOOT_SRAM the **196,292-byte AXI load span** includes the 1,380-byte `.data`
initialization copy as well as executable VMA sections. `.data` also occupies
DTCM after startup. Thus AXI use is not just ordinary BSS, and total RAM must
include loaded instructions, constants and their initialization copies. The
unchanged upstream SRAM linker reserves the final 32 KiB of AXI for loader work:
app load capacity **480 KiB / 491,520 bytes**. DTCM data shares 128 KiB with the
downward-growing main/interrupt stack. Its default heap starts at linker `end`,
**`0x30008000` in D2**, not in DTCM; the 256 KiB region is not a measured heap budget.

BOOT_QSPI leaves instructions/constants external and `.data`/BSS in AXI. The
upstream QSPI region begins 256 KiB into the 8 MiB device, giving **7,936 KiB /
8,126,464 bytes** of application capacity. The first 256 KiB is excluded by the
layout, not counted as app usage. Heap starts at **`0x2400369c`** in AXI. The
linked newlib/nosys `_sbrk` disassembly grows from `end` without a region bound;
neither diagnostic demonstrates safe allocation/exhaustion or exception handling.
This requires a separately reviewed bounded runtime heap before deployment.

Upstream `BOOT_APP` omits startup `SystemInit()` for both loader images; the
bootloader supplies the initial handoff/VTOR/MSP setup. This explains their
160-byte reduction from the failed internal-flash diagnostic, not a feature cut.
SRAM execution consumes on-chip RAM but avoids QSPI instruction-fetch dependence;
QSPI leaves more on-chip RAM but adds cache/bus/flash contention concerns. Worst-
case callback timing and cache misses must be measured for each before selecting
a production execution layout. No capacity or real-time conclusion follows from
a successful ELF link alone.

Unmodified upstream linker SHA-256 values:

- SRAM: `697e963cd517992d69f96c89bf1e14cdd4477615ddbb86fb34f0eb7b1e3ceee9`.
- QSPI: `09c1283f2f7df9c7e83a6a90beb006088b561a3aac12d3ac35c6cc6f6db5c65a`.

### Bootloader and Seed3 compatibility evidence

The pinned SDK's `core/Makefile` selects these exact upstream linkers and adds
`BOOT_APP`. Both bootloader images are programmed through the Daisy loader at
`0x90040000`; ROM DFU by itself cannot install them there. The SDK bundles
`core/dsy_bootloader_v6_4-intdfu-2000ms.bin`, **130,188 bytes**, SHA-256
`76296b6bd625e5e5dbeba2431619a0ed90ae2c0a3a99258df00a64a0b8b9ec35`.
The recipe verifies that hash but **does not flash, rebuild or run the loader**.
Its internal-flash usage/RAM while updating is additional to these app reports.

Audited [official bootloader source](https://github.com/daisyaudio/DaisyBootloader/tree/8b279a8e25922b786c24a6cbd1d499cdc1e76934)
at `8b279a8e25922b786c24a6cbd1d499cdc1e76934`: its distributed binary matches the
SDK's bundled hash. `shared/bootloader.cpp::FillTargetMemory` copies SRAM images
into the 480 KiB execution buffer or selects QSPI; `startup_process` sets VTOR/MSP
and jumps, initializing mapped QSPI when needed. Its libDaisy gitlink is the
**different** `24be4afea00d71988019fe8dd9fd9fc8fc5bf72c`. Binary/source build
equivalence has not been reproduced. The binary filename says v6.4 while source
BootInfo advertises the v6.1 handoff enum; do not infer a new protocol from the name.

[Official instructions](https://docs.daisy.audio/tutorials/_a7_Getting-Started-Daisy-Bootloader/)
document both layouts and the 480 KiB/7.75 MiB limits. Seed3's datasheet identifies
STM32H750, Seed-compatible external pinout and PC7 LED; the current pinned SDK
explicitly detects Seed3/TAC5242 and shares the Seed QSPI configuration
(IS25LP064A, PF8/PF9/PF7/PF6/PF10/PG6). The audited loader's default QSPI setup
uses that same device/pin mapping. These establish software/layout alignment.
**The loader README names Seed, Seed2 DFM and PatchSM, not Seed3 explicitly**;
neither the datasheet's pin-compatibility claim nor ELF linkage verifies internal
QSPI wiring/part revision, loader codec initialization, USB recovery, or actual
Seed3 boot compatibility. Manufacturer confirmation and measured boot/cold-start/
recovery on the exact Seed3 revision remain open. These are linkable candidate
configurations, not validated flashing instructions or a supported-board claim.

### Does JSON need to execute on Daisy?

No architectural requirement found in the current code forces that placement:
`Engine::load` accepts the compact native `Patch` and does not depend on JSON.
Canonical versioned JSON remains the authoring/save/load authority selected in
ADR-001. It need not be the DSP processor's in-memory representation or execute
on every processor in a multi-controller system.

The planned ESP32-S3 **could** own canonical JSON decode/validation, composition,
serialization and preparation, then send an explicitly versioned compact native
plan to Daisy. This would reduce Daisy parser/heap/exception work without changing
correspondence rules or dropping canonical functionality. It is a proposal, not
an implemented or benchmarked ESP32 feature. We have not measured ESP32 RAM/CPU.
Do not transmit a raw C++ `Patch` memory dump: enum sizes, ABI, endianness and
padding are not a wire contract. A follow-up must specify bounded framing, units,
processor capability/version negotiation, actual Daisy sample-rate handshake,
integrity/error handling, DSP-side native validation, owned storage and safe
activation outside the callback. The existing preparation code depends on the
validated document API; this ticket does not create a transport decoder.

Keeping decode on Daisy is also viable for evaluation with the two linkable
larger layouts, at the cost of dynamic control-side allocation and runtime/boot
validation. Prefer investigating controller ownership before replacing the parser
or rejecting JSON because of BOOT_NONE's limit. Neither processor ownership nor
the production boot layout is decided by this build gate. Four persistent presets
and standalone playback remain requirements; storage ownership, boot/reconnect
behavior and power-loss guarantees still need a separate specification.

## Warnings, artifacts and remaining bring-up

Build logs retain upstream warnings: default interrupt handler FP-register
attribute warning; newlib/nosys unsupported syscall stubs; ELF RWX LOAD segment
from the upstream script. They are not suppressed or described as hardware-safe.
Minimal callback does no syscall work. Investigate handler/segment/runtime needs
before on-device acceptance; static linking is the current gate only.

The ARM CI job (15-minute limit, pinned checkout/upload actions) performs this
same build and retains evidence for seven days: ELF/hex/bin, map, disassembly,
section/symbol/size reports, application `.su` data, preparation object/header,
all three image reports, internal-flash JSON-probe map/error log, input/version
manifest and dependency license notices.
The existing host Debug/Release workflow remains unchanged. Run links and exact
local commands/results are in the PR. No target sanitizers/emulation were run.

For actual bring-up, obtain the specified Seed3 module and preferably the official
pedal development kit, a known data-capable USB-C cable, regulated/current-limited
power and a verified ROM-DFU/debug programming/recovery path. Confirm board/codec
detection, configured rate and CPU clock on the device. Verify SDRAM/QSPI with
separate tests before using them. Begin with safe known **line-level** test input,
monitor output on suitable measurement equipment, then measure latency, noise,
clipping, level bounds, callback load/underruns, memory high-water and power/heat.

Do not assume a passive guitar can connect directly: the datasheet line-input
example specifies roughly **20 kOhm** impedance. Validate an instrument-suitable
buffer/impedance, coupling/bias/protection, gain/headroom, output load and bypass
arrangement before guitar/amp connection. No electrical safety, pickup-loading,
sound-quality or manufacturing-form claim follows from this build.

Follow-up gates remain separate: measured on-hardware signal path, live patch
ownership/activation, and one actual correspondence-to-audio trace. This build
gate supports that work without making hardware the product's defining purpose.
