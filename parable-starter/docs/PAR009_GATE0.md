# PAR-009 Gate 0 — audit before implementation

2026-11-11; main `0f84cb022dd5567fae8a62644921c1d61a079e71` (merged #27).
Required rules/constitution, ADR-001, PAR-008 contract/evidence, firmware scripts,
pins, USB/power notes and SDK implementations read; none missing. Clean baseline:
Debug **18/18**, Release **18/18**, pinned ARM **four images linked**, expected
codec BOOT_NONE overflow 88,800 B. No hardware used.

**Reuse, not another architecture.** Keep the pinned xPack/libDaisy/driver locks,
unmodified upstream linkers, serial Engine, exact RAT composite and host evidence.
Current firmware lacks standalone LED isolation, visible startup failures, runtime
measurements/retrieval and a founder recovery procedure. Add three labeled
BOOT_NONE first-boot targets; preserve original bypass/RAT/codec regression builds.
No DSP/patch-format edits, software bootloader installation or migration.

**Official audit.** [Seed3 hardware](https://docs.daisy.audio/hardware/Seed3/)
specifies USB-C power/data. [Programmer](https://flash.daisy.audio/) and
[troubleshooting](https://docs.daisy.audio/troubleshooting/) specify BOOT held,
RESET pressed/released, then BOOT released. Programmer's Micro-USB wording is
legacy; its separate Bootloader tab is not this task. [Bootloader guide](https://docs.daisy.audio/tutorials/_a7_Getting-Started-Daisy-Bootloader/)
and [official announcement](https://daisy.audio/blogs/seeds-n-circuits/daisy-bootloader-now-open-source)
distinguish immutable STM32 ROM entry from a replaceable internal-flash Daisy
loader and SRAM/QSPI apps. Pinned `core/Makefile` programs BOOT_NONE at
`0x08000000`, alternate 0. No `program-boot` or external-flash write is authorized.

Pinned SDK `src/daisy_seed.cpp` identifies Seed3 through `CheckBoardVersion()`
(PH6 strap), sets its onboard LED to PC7, and `Init()` also initializes QSPI,
SDRAM and audio. [Official Blink](https://github.com/daisyaudio/DaisyExamples/blob/728e93d2fa213df4e2b74160fa02f0f146fede87/seed/Blink/Blink.cpp)
uses SetLed/Delay, compatible with this pin; reuse only the system/GPIO subset for
LED isolation, not full audio Init. No external/header wiring inferred.

**Measurement/retrieval.** Compare pinned `util/CpuLoadMeter.h` (TIM ticks,
floating smoothing) with [Daisy's DWT precedent](https://community.daisy.audio/t/solved-how-to-do-mcu-utilization-measurements/1236)
and pinned CMSIS `core_cm7.h`. Select integer CYCCNT entry/end sampling, explicit
unsigned-wrap interval contract, counter availability check and RCC-derived
`SystemCoreClockUpdate()` frequency. No hardcoded CPU rate, percentile or latency.
Capture a bounded first-30,000-callback summary, then freeze and publish it once
to control using a lock-free completion flag; DSP continues. This avoids racing
mutable statistics and keeps USB outside the captured window. Include stopped
RAT-load cycles/wall ticks if the interval is valid; do not optimize preparation.

Pinned `logger.h` eventually busy-waits; `CDC_Transmit_FS` dereferences class data
without a null check. Reuse existing USB CDC after capture, with one immutable
report buffer, configured/non-null checks and a single nonblocking send attempt
under a short **control-side** IRQ guard. No callback IO, USB protocol, mutable
in-flight buffer or SDK modification. Report any ROM overflow/retrieval blocker
rather than delete DSP. Physical CDC/driver behavior and guard latency are unproven.

**Older Mac.** Arduino's [official package index](https://downloads.arduino.cc/packages/package_index.json)
pins `dfu-util` 0.11-arduino5 Darwin AMD64 archive SHA-256
`9e576c6e44f54b1e921a43ea77bcc08ec99e0e4e0905f4b9acf9ab2c979f0a22`.
Downloaded/checked: x86_64 Mach-O, minimum macOS **10.12**, only system dylibs/
frameworks. This is a concrete native 10.13 candidate, not a Mac execution or USB
test. Supply version-check commands plus Ubuntu 24.04 native fallback requiring
access to that OS. Firefox has no native WebUSB; current Chrome/Homebrew cannot
be assumed on High Sierra. Do not make an installer or remove quarantine blindly.

**Licensing/gates.** No external model code imported. SDK/Blink MIT and existing
CMSIS/ST per-file notices retained; forum is reference only, no code copied.
dfu-util is a separate GPL host utility, not linked/distributed DSP. Prove Debug/
Release and math/status simulations, original lab hashes, actual ARM calls/sections/
static frames and source/bin hashes in CI. Founder guide must gate carrier rails,
orientation and USB/DC power coexistence on #26; never invent carrier pin numbers.
Stop for a larger decision if BOOT_NONE or safe reporting cannot fit this narrow
adaptation. **Audit clears implementation; all physical gates remain pending.**

Link/source verification note: pinned `usbd_conf.h` uses malloc/free for USB class
state. This occurs after capture, outside AudioCallback; global heap/USB IRQ
robustness is not established by these hooks and requires physical testing.
