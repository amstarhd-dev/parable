# Seed3 runner — Gate 0 and handoff design

Scope: benchmark images from open PR #83, not production boot or DSP. Parent
`3d091127f154847c6938ff909ceecc1ca942ac85`; no parent PR is merged here.
Repository AGENTS, Constitution, README, implementation status, PAR-009 recovery
procedure and resource harness were read. Pre-edit resource suite: 8/8 passes,
including actual kernel compilation and the sanitized mixed chain.

## Prior art and pinned definitions

- [ST AN2606](https://www.st.com/resource/en/application_note/an2606-.pdf),
  retrieved Rev 71, November 2026, section 63/table 145, printed page 342 onward.
  H74/H75 ROM entry is **0x1FF09800**, distinct from system-memory region base.
  PDF SHA256 `dd8e48f0ae32ae3fb168d9d8da72092714289b9e410d2fcb5964b10dfc86396a`.
- [ST AN3156](https://www.st.com/resource/en/application_note/an3156-usb-dfu-protocol-used-in-the-stm32-bootloader-stmicroelectronics.pdf),
  upload and Set Address Pointer semantics. PDF SHA256
  `0dccaa3e76b339cf42eb9d021a21c73f3e256146500486158d5049594747ffb5`.
- [ST application-to-ROM guidance](https://community.st.com/stm32-mcus-60/how-to-jump-to-system-bootloader-from-application-code-on-stm32-microcontrollers-71):
  peripheral/clock/interrupt state must be suitable for ROM; MSP and entry come
  from the ROM vector table. Its running-application example is not copied.
- [Pinned libDaisy](https://github.com/daisyaudio/libDaisy/tree/1289a000bc29dae1bc0ed4129cfa2df40ef71574):
  `System::ResetToBootloader(STM)` drives PG3 high for 10ms then resets. This is
  useful prior art but electrically dependent on BOOT-pin coupling. Daisy custom
  bootloader modes require a different installed loader; neither is assumed here.
- Pinned `core/startup_stm32h750xx.c`: copy data, zero `.bss`, call `SystemInit`,
  call constructors, main. GNU `--wrap=SystemInit` provides an early hook without
  replacing startup or changing any SDK files. Used **only with --usb-control**.
- CMSIS Device/ST commit `6dac8c24d7b38ab20806d27dd7d8285a6433b8f7`:
  `UID_BASE=0x1FF1E800`, `FLASHSIZE_BASE=0x1FF1E880`, DBGMCU base
  `0x5C001000`; RCC reset flags and CMSIS cache/barrier/reset definitions.
- [dfu-util 0.11 source](https://dfu-util.sourceforge.net/releases/dfu-util-0.11.tar.gz),
  SHA256 `b4b53ba21a82ef7e3d4c47df2952adf5fa494f499b6b0b57c58c5d04ae8ff19e`,
  `src/dfuse.c:dfuse_do_upload`: `force` bypasses the host memory-map check for
  reads outside the flash alternate setting. The runner uses it **only on -U**,
  with three fixed addresses and fixed 12/4/2-byte lengths. No force download,
  mass erase, read-unprotect or option-byte command is generated.

## Selected approach

After complete report transmission, a foreground parser accepts `HELLO` with a
fresh host nonce, reports actual silicon identity and accepts exactly
`ENTER_DFU <artifact-id> <nonce>`. CLI explicit authorization is also required.
This protects against accidental commands/wrong firmware; it is not authentication
against a hostile USB host. A USB disconnect cancels the entire report/authorization session. The USB IRQ copies bounded bytes only. Overflow fails
closed; commands received before report completion are discarded.

After the authorization response finishes transmitting, firmware waits 100ms,
disconnects CDC, waits 100ms, clears reset-cause flags, writes a cache-line-aligned
one-shot RAM marker, cleans that line if needed and requests SYSRESETREQ. No flash
or backup-register storage is used for the marker.

A supplementary linker section reserves **32 NOLOAD bytes in AXI SRAM**, outside
`.bss`. The original SDK memory regions and 128 KiB FLASH limit are unchanged.
The wrapper consumes the marker before branching, requires a software reset
without POR/BOR and validates ROM vector addresses/alignment. It sets VTOR,
barriers, MSP, CONTROL and branches with interrupts enabled. Normal boot falls
through to the original SystemInit. MSP replacement and branch are contiguous
assembly: no compiler epilogue can use the replaced stack. No HAL, FPU, USB,
audio or constructors execute on the requested early-ROM path.

This avoids attempting to deinitialize a live audio/USB/PLL environment before a
jump. The mailbox retention, ROM revision, USB detachment and actual Seed3 boot
must **still be proven physically**. Link/disassembly inspection cannot do that.
Ordinary BOOT + RESET enters ST ROM directly; this wrapper is never executed
on that path. Cold boot does not arm a transition. No option bytes or production
firmware/startup files change.

## Host admission and residual risks

Every download checks the exact selected ROM serial/path, alt0 128 KiB layout,
UID, device-family ID and factory flash-size word. ID 0x450 alone names a family;
128 KiB flash and firmware's Seed3 board check complete the expected target
constraints. This is identification evidence, not cryptographic board attestation.
CDC identity must match the ROM reads before any reset command is sent.

**ROM read accessibility is unproven on the physical board.** If ROM cannot upload
UID, DBGMCU or flash-size words, the runner stops **before flashing**. Do not bypass
that safeguard. Capture the diagnostic for a separately reviewed identification
strategy. No unsupported claim is made that AN3156 guarantees peripheral reads on
every ROM revision. Likewise exact DFU layout and High Sierra IORegistry behavior
are unverified on the user's hardware.

A separate low-cost RAT probe must return a complete report and re-enumerate as
the same ROM silicon after software reset. Only then is a local transition
receipt issued. Batch requires the receipt's exact kit hash, tool hash, handoff
version, runner source hash, silicon and port/serial. A receipt is operator-owned evidence, not a
signed laboratory attestation; do not edit or fabricate one. A rebuilt kit needs
a fresh probe. Enumeration retries are bounded; writes are never retried. Any
failure leaves the app or ROM available for ordinary manual recovery.
