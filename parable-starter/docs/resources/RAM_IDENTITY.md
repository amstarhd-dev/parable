# Separate Seed3 RAM-only identity test v0.1

**Built and host-tested; NOT physically tested.** This is an isolated experiment
for unmerged PR84. It cannot flash firmware, issue a benchmark transition receipt,
or unlock the full runner. The old v1.1 identity backend remains blocked.

The helper reads silicon identity using native CPU instructions and returns a
128-byte nonce-bound record through RAM. The host uploads that RAM record **after
ROM returns**. There is no application USB stack, CDC discovery, audio, libDaisy
library, dynamic allocation, C runtime, option-byte programming or flash driver.

## Physical test — one command after initial ROM entry

Download the separately versioned RAM identity ZIP linked in PR84 and extract to
a **new folder**. Read this guide before running. Verify the ZIP hash against the
PR / checked-in `ram-identity/kit.json`; inside it run
`shasum -a 256 -c SHA256SUMS`. Retain the previously verified Intel dfu-util
0.11-arduino5 executable (SHA256
`a200781d74555c0d9c4b379fc7d5872deed6e13bf3988fc9d6313210090a8f43`).
Python3.6+ standard library is sufficient; actual High Sierra execution is untested.

Use **only the known physical Seed3**, with PG3 unconnected to any external
circuitry; this test deliberately drives PG3 as the BOOT signal, following pinned
libDaisy's STM bootloader reset method. Disconnect downstream audio equipment.
Close other USB tools and disconnect other STM32 devices. Ordinary BOOT/RESET
recovery must already work. Hold BOOT, press/release RESET, release BOOT to enter
ROM if necessary. Run the verified `dfu-util -l` and copy this board's ROM serial.
Do not infer flash capacity from `16*128Kg`.

From the extracted folder, run this **single test command**, substituting the
verified dfu-util path and observed ROM serial:

```sh
python3 ram_identity_test.py --manifest manifest.json \
  --manifest-sha256 060b62c7796c1b65fc16e04e0aab54d01edff30d1f6d11ce9424566cc6efe6ba \
  --dfu-util /path/to/darwin_amd64/dfu-util --serial YOUR_ROM_SERIAL \
  --output ram-probe-001 --authorize-seed3-pg3-ram-test
```

After starting the command, leave buttons, power and cable alone until it exits.
Manual intervention invalidates an automatic-return claim; the software cannot
attest button presses.

It performs **two rounds**, each with a fresh128-bit nonce:

1. Verify the independently supplied manifest hash, helper source identity,
   binary SHA256, fixed RAM layout, size and initial vectors. Identify one ROM
   USB device with the exact selected serial/path and expected internal-flash
   interface. No direct ROM read of UID, DBGMCU or factory-size addresses occurs.
2. Download only the helper and128-byte request into documented RAM, then read
   back and compare **every byte** before executing anything. Hash verification
   does not establish the chip's identity; it establishes the RAM program being
   requested. An unexpected ROM restriction stops the test.
3. Execute via DfuSe leave at the verified RAM vector base. The helper captures
   the full96-bit UID, DBGMCU IDCODE and16-bit factory flash-size register. It
   commits a CRC-protected response with the same nonce/helper identity and then
   requests a full system reset with the Seed BOOT pin charged high.
4. Require an observed USB disappearance or a changed enumeration generation,
   and return of the **same ROM serial/path/interface**. Read the RAM mailbox,
   validate its nonce/CRC/startup fields, chip family0x450 and factory128KiB size.
5. Repeat with another nonce. Require the complete native silicon identity from
   both rounds to match. Only then write `identity-observation.json`.

Keep the complete output folder. It contains every dfu-util argument list and
combined output/error, request/readout records and parsed identities. Send the
full first error if it stops. Use a new output folder for any separately reviewed
retry; the application never automatically retries a write or execution request.

**On missing ROM return, invalid readback/nonce/identity or other unexpected
behavior: STOP.** No flash operation follows. Recover using normal BOOT/RESET;
do not force onward, alter option bytes or hand-edit a receipt. If PG3 return
fails, the existing flash application may boot instead; this is a failed test.
RAM retention across reset and ROM startup is unproven. A cleared/overwritten
mailbox is rejected. Power cycling removes the volatile test state.

A successful observation establishes this RAM identity/reset experiment only.
It does **not** prove the benchmark images' separate early-marker handoff, batch
operation, measured Daisy performance or full-runner admission. No physical
success report is included in this PR.

## Electrical, startup and memory audit

[AN2606 Rev71](https://www.st.com/resource/en/application_note/an2606-.pdf),
section63/table145 and section102/table239, documents H74/H75 ROM configuration,
workspace and available RAM. AXI code/mailbox are above both the20KiB and208KiB
ROM workspace sizes listed there. The DTCM stack is above ROM's reserved area.
Only the following ranges are used:

| Region | Address | Reservation / actual usage |
| --- | --- | --- |
| Code and vectors | 0x24040000 | 4096B region;1416B BIN |
| Vectors | 0x24040000 | 166 entries /664B; base1024-byte aligned |
| Mailbox | 0x24041000 | 128B NOLOAD |
| Stack | 0x2001FC00–0x20020000 | 1024B NOLOAD; compiler static frame40B |

The stack end is exclusive; MSP starts one byte past the reservation and pushes
downward into it. All ELF LOAD segments lie within those documented RAM regions; the raw binary
contains code/vectors only. No flash/QSPI/SDRAM load region exists. Startup sets
MSP and privileged MSP CONTROL before any C++ frame. No `.data`, `.bss`,
constructors, HAL or SDK startup executes. Reserved core vectors are zero; all
other exception/IRQ entries point to Ram_Fault, which masks interrupts and halts. Static frame
size is not physical stack high-water evidence.

[AN3156 Rev19](https://www.st.com/resource/en/application_note/an3156-usb-dfu-protocol-used-in-the-stm32-bootloader-stmicroelectronics.pdf),
sections4.1/5.1/5.5, describes RAM reads/writes and RAM execution via leave. Its
pending-USB/core-reset requirement is addressed by **never enabling interrupts
in the helper**, rather than starting an application USB stack. Startup masks
interrupts, disables SysTick, disables/clears all150 external IRQs, clears pending
PendSV/SysTick, disables/cleans caches using pinned CMSIS, sets VTOR, and pulses
USB peripheral resets. It records IPSR because ROM may leave from handler mode;
no attempt is made to write read-only NVIC active bits. A full SYSRESETREQ occurs
before ROM initializes and enables USB interrupts again. The compiled helper
contains no `cpsie`, SVC, indirect calls or unreviewed runtime calls.

The return uses [pinned libDaisy System::ResetToBootloader(STM)](https://github.com/daisyaudio/libDaisy/blob/1289a000bc29dae1bc0ed4129cfa2df40ef71574/src/sys/system.cpp)
as target-specific prior art: PG3 push-pull high, charge delay, system reset.
The implementation changes only PG3 GPIO fields and waits five million bounded
NOP-loop iterations, exceeding the SDK10ms delay even at480MHz. It uses no tick
IRQ. Pin coupling/charge time on this physical Seed3 must still be tested.
No boot-address options, flash controllers or persistent reset marker are used.

The disassembly shows all three volatile UID word reads and the native DBGMCU
word/factory-size halfword reads. Reviewed stores target the mailbox/stack,
SCB/SysTick/NVIC, RCC USB reset/GPIO clock and GPIOG pin3. No FLASH-controller
literal or external runtime symbol occurs. These checks are source/link evidence,
not a formal proof of hardware behavior.

## Transport safeguards and wire contract

The host transport accepts only the exact code/mailbox addresses above and fixed
maximum lengths; it rejects flash, option-byte, peripheral, shifted or overflowing
addresses. `:force` on RAM reads/downloads bypasses only dfu-util's advertised
**flash** map check. These RAM addresses are separately documented by ST; force
is not used to evade a ROM restriction. Any ROM rejection is fatal. The pinned
[upstream dfu-util0.11](https://dfu-util.sourceforge.net/releases/dfu-util-0.11.tar.gz)
`dfuse_dnload_element` skips erase when the address has no memory-map segment;
the chosen internal-flash descriptor contains no RAM segment. Leave uses DfuSe
command mode with no `-D`; no erase/unprotect/option-byte command is constructed.

The128-byte little-endian record has32 words: magic, schema1, word-count32,
status0, four nonce words, eight SHA256 helper-identity words, three UID words,
IDCODE, factoryKiB, RCC reset flags, entryCCR, entryVTOR, helperVTOR, PG3 marker3,
IPSR, CONTROL0, PRIMASK1, two reserved zeros and CRC32. CRC covers bytes4..123;
response magic is committed last with barriers while data cache is disabled.
CRC is corruption detection, not authentication. Requests initialize unused
fields to zero. Only a complete artifact/nonce-matching response is accepted.

Original-device correlation is unchanged ROM serial/path/interface before/after
execution plus matching full native UID across two independent challenges. This
is instrument identification, not cryptographic protection against cloned USB
identities. Neither this result schema nor its filename is a full-runner receipt;
`flash_programming_permitted` is always false.

## Reproduce and evidence

```sh
python3 -m unittest discover -s parable-starter/tests -p test_seed3_runner.py -v
python3 -m unittest discover -s parable-starter/tests -p test_ram_identity.py -v
python3 parable-starter/tools/resources/ram_identity/build.py \
  --compiler "$ARM_GXX" --libdaisy "$LIBDAISY" --output build/ram-identity
python3 parable-starter/tools/resources/ram_identity/package.py \
  --build build/ram-identity --output build/ram-identity-package --libdaisy "$LIBDAISY"
```

Local verification: **19 existing runner regressions and11 new RAM tests pass**;
the actual portable C++ wire functions run under ASan/UBSan. No unrelated
DSP/SPICE/audition suite was repeated.

The build enforces the existing xPack14.2.1 / exact libDaisy/CMSIS pins, including
under optimized Python, and uses freestanding Cortex-M7 soft-float with no runtime
libraries. The code source hashes, ELF/BIN hashes, link map, complete disassembly,
segments, symbols, static frames and startup audit are packaged. CI rebuilds this
helper and runs the focused simulations plus the actual portable C++ wire code
under ASan/UBSan. The15 benchmark images were **not rebuilt or changed**.

Hardware return, RAM survival, PG3 behavior, High Sierra operation and factory
register values remain **unmeasured** until the founder performs this test. The
full runner stays blocked; integration is a later independently reviewed change.
