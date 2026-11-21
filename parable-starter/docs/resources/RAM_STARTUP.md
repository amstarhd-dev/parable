# RAM startup diagnostic v0.2 — launch, manually recover, collect

**Diagnostic candidate, not a working identity bootstrap. PR84 remains unmerged.**
The founder demonstrated RAM transfer/readback and mailbox retention after manual
BOOT/RESET. QDIP remained unchanged and response fields stayed zero. That proves
neither entry into the old helper nor completion of any native identity read.
Lost GETSTATUS is not execution evidence. No host timeout was increased.

## Concrete startup correction and what is still unknown

[ST AN2606](https://www.st.com/resource/en/application_note/an2606-.pdf),
section63/table146, documents a jump limitation for H74/H75 V9.x bootloaders:
the application SP must be below RAM end minus16 bytes. v0.1 used **0x20020000**,
the DTCM end boundary, violating that documented guidance. v0.2 uses
**0x2001FFC0** in both the vector and explicit MSP setup: aligned, inside valid
RAM, with64 bytes of headroom. The actual board's bootloader revision and causal
instruction failure were not captured; this is a confirmed image defect and a
source-backed correction, **not a claim that it explains every physical symptom**.

DTCM remains the stack location so cache maintenance cannot invalidate a live
write-back AXI stack. VTOR now changes before the explicit MSP/CONTROL transition
and before C++ startup. The first executed instruction masks IRQs. The first two
markers use no stack, calls, MSP/CONTROL writes, HAL, USB driver or C++ prologue.
A naked fault handler is installed early and never dereferences a suspect stack.
It captures IPSR, LR/EXC_RETURN, MSP/PSP/CONTROL/PRIMASK/BASEPRI, CFSR/HFSR/DFSR,
MMFAR/BFAR with validity flags, SHCSR/AFSR/ICSR/VTOR/CCR. Host-side frame decoding
reads only a fixed approved DTCM window, checks EXC_RETURN/range and avoids frame
interpretation on stacking errors. A recovered PC is labelled a **candidate**:
RAM after reset is not independent proof of frame freshness.

[AN3156 section5.5](https://www.st.com/resource/en/application_note/an3156-usb-dfu-protocol-used-in-the-stm32-bootloader-stmicroelectronics.pdf)
describes leave into a RAM vector table and the USB/core-reset interrupt caveat.
All diagnostics keep interrupts masked; default probes halt for manual recovery.
Only the separately authorized reset probe performs a system reset before ROM
can enable USB IRQs again. No exception-return trick or NVIC active-bit write is
used to force thread mode. Markers record the actual IPSR and CONTROL state.

### Does fileless `:leave` execute RAM code?

The audited [upstream dfu-util0.11 source](https://dfu-util.sourceforge.net/releases/dfu-util-0.11.tar.gz)
contains an explicit command-mode path for `-s` without `-D`:
`main.c` selects download-command mode; `dfuse_do_dnload` skips file download;
`dfuse_do_leave` sets the address pointer and sends a zero-length DNLOAD followed
by status handling. Thus fileless leave is an intended request path, not a file
transfer requirement inferred from examples. Source audit is not proof of this
ROM's actual branch. The pinned Arduino executable is hash-verified; upstream
source is not claimed byte-equivalent to that vendor binary.

The diagnostic permits a separate, explicit `--handoff download` experiment with
the **same chosen image**. It performs the normal verified preparation, then
re-downloads that immutable, hash-checked RAM image with `:leave`. The final
combined transfer cannot be read back after execution; earlier preparation was
read back in full. Neither a zero process exit nor lost GETSTATUS is success.
There is **no automatic fallback/retry** between handoffs. Collect one attempt's
evidence before authorizing another in a fresh session. No direct-PC guess,
option-byte change or unsupported-address workaround is offered.

## Three minimal images

| Probe | Behavior | Expected final marker |
| --- | --- | --- |
| `entry` | Stackless entry markers, IRQ mask, deliberate halt | 2 |
| `startup` | Corrected startup, cache/IRQ/USB cleanup, native identity record, halt; no PG3 | 17 |
| `reset` | Same startup plus staged PG3 clock/setup/delay/SYSRESETREQ | 21 before reset request |

Initial MSP is0x2001FFC0; DTCM reservation0x2001FC00–0x20020000 is end-exclusive.
Code/vectors occupy at most4096B at0x24040000. Mailbox is128B at0x24041000.
Trace is1024B at0x24042000, all within AN2606 table239's available RAM and above
ROM workspace. No data/BSS/constructors or flash/QSPI/SDRAM LOAD sections exist.
Code images are944/1908/2060 bytes. Only the reset image contains the PG3 path.
The15 benchmark images and the v0.1 helper binaries have not been rebuilt.

Each progress record occupies its own32-byte cache line: magic, ordinal,
inverse ordinal, IPSR, MSP, CONTROL, PRIMASK and commit tag. Prior normal-memory
writes are drained with DSB, the line is cleaned to PoC through DCCMVAC, then
DSB/ISB completes publication. Fault capture cleans all four of its cache lines.
The host initializes and verifies a fresh trace containing nonce/artifact context
and zero marker slots before leave. The earliest sentinel deliberately avoids
request validation so it can show entry before C++ or stack operations. These are
session-correlated diagnostics, not authenticated identity/admission records.

Zero markers means **no durable entry evidence**, not proof that no instruction
ran: failure before the first store/clean, privilege restrictions, stacking
lockup or recovery clobbering remain possible. Fault capture before VTOR is
installed, on double fault/lockup, or during the capture itself is best effort.
Partial/gapped/corrupt traces are retained and cannot authorize progression.

## Physical procedure — begin with `entry`

Download the separately versioned startup ZIP linked in PR84 into a fresh folder.
Check its independently published ZIP hash and run
`shasum -a 256 -c SHA256SUMS`. Use the same previously verified Intel Mac
0.11-arduino5 dfu-util, Python3.6+, and the known Seed3 only. Disconnect other
STM32 devices and audio equipment. Never use these images as flash firmware.

Set the ROM serial from `dfu-util -l`, the executable path and the reviewed
manifest hash. Start in a fresh ordinary BOOT/RESET ROM session. Launch once:

```sh
python3 ram_startup_test.py launch --manifest manifest.json \
  --manifest-sha256 332b7e1ac3e8cd223b3a0ab9cb5572139758800835166876d2680adbffccc4d6 \
  --dfu-util /path/to/darwin_amd64/dfu-util --serial YOUR_ROM_SERIAL \
  --probe entry --handoff command --output entry-launch-001 --authorize-ram-diagnostic
```

The tool hash-verifies all images, selects the exact ROM serial/path/interface,
loads only approved RAM regions and checks every byte of code, request and trace
before leave. The session is saved before the execution attempt. If leave loses
GETSTATUS it exits nonzero, preserves the error and reports execution **UNKNOWN**;
it does not turn that error into success or wait for an automatic return.
If any preparation/readback fails, no leave is attempted.

The entry image intentionally halts. **Now use manual BOOT/RESET recovery**, as
requested for this diagnostic. Do not relaunch or re-download anything: that
would overwrite the evidence. Collect it with this read-only command:

```sh
python3 ram_startup_test.py collect --manifest manifest.json \
  --manifest-sha256 332b7e1ac3e8cd223b3a0ab9cb5572139758800835166876d2680adbffccc4d6 \
  --dfu-util /path/to/darwin_amd64/dfu-util --serial YOUR_ROM_SERIAL \
  --session entry-launch-001/session.json --output entry-readout-001
```

Keep **both complete folders**, especially session.json, command/error logs,
trace.raw, mailbox.raw and collected.json. The collector verifies the same
original ROM USB identity, preserves raw bytes before interpretation and never
writes RAM, leaves ROM or flashes. A valid marker2 is evidence that entry code
ran and completed the two durable stores. No marker is not permission to guess
another address or increase a timeout. Report the files for review.

After reviewing a clean entry result, `--probe startup` requires
`--previous-report entry-readout-001/collected.json`. Use a new launch/output pair,
manually recover and collect in the same way. It should reach17 and a complete
nonce/CRC-bound native UID/chip/capacity report. An incomplete/faulted previous
probe cannot authorize the next one. The `reset` probe additionally requires the
startup readout with a valid native record and `--authorize-pg3-reset`; only then
may PG3 be driven. Do not proceed after unexpected behavior.

For an independently reviewed handoff comparison, repeat **entry** in a new
session with `--handoff download`; no mode auto-switches after an error. Do not
reuse an output folder or substitute a different board/cable/USB port.

Every collected report keeps `identity_verified=false`,
`flash_programming_permitted=false` and `automatic_rom_return_verified=false`.
Manual recovery cannot prove automatic PG3 return. Marker21 proves reaching the
point before SYSRESETREQ, not that reset or ROM enumeration succeeded. No report
is a full-runner receipt; all original silicon/128KiB/no-flash gates remain.

## Marker map and reproducible checks

1 entry;2 sentinel halt/pre-VTOR;3 post-VTOR/pre-MSP;4 post-MSP/pre-CONTROL;
5 post-CONTROL/pre-C++;6 C++ reached/pre-core reads and IRQ cleanup;
7 pre-D-cache disable;8 post-D-cache/pre-I-cache;9 post-I-cache/pre-USB reset;
10 post-USB/pre-request validation;11/12/13 pre-UID0/1/2;14 pre-DBGMCU;
15 pre-factory capacity;16 pre-record/CRC;17 native record committed;
18 pre-GPIO clock;19 pre-PG3 setup;20 pre-charge delay;21 pre-SYSRESETREQ.
A marker names the point **before** the next operation. Use the included exact
ELF/disassembly and fault status to locate instructions between adjacent markers.

```sh
python3 -m unittest discover -s parable-starter/tests -p test_ram_startup.py -v
python3 -m unittest discover -s parable-starter/tests -p test_ram_identity.py -v
python3 parable-starter/tools/resources/ram_startup/build.py \
  --compiler "$ARM_GXX" --libdaisy "$LIBDAISY" --output build/ram-startup
```

The build checks exact existing toolchain/SDK/CMSIS pins, complete vectors,
strictly interior MSP, legal LOAD segments, no runtime dependencies, no IRQ enable,
stackless sentinel and stackless fault capture. Host simulations cover recovered
trace/fault/frame decoding, missing entry, stale/corrupt/gapped records, leave
errors without success, immutable read-only collection, progression gates and
RAM-only command bounds. Hardware entry, native completion and automatic return
of **v0.2 remain unproven**. Musical/DSP tests and CPU utilization are unrelated
and were not claimed or remeasured.

Package the reviewed build without rebuilding earlier images:

```sh
python3 parable-starter/tools/resources/ram_startup/package.py \
  --build build/ram-startup --output build/ram-startup-kit --libdaisy "$LIBDAISY"
```

The kit includes independently checked BIN/ELF hashes, vectors, disassembly,
linker maps, static stack estimates, source and host hashes, CMSIS licenses and
SHA256SUMS. Its README carries the reviewed manifest digest. Compare the ZIP
digest against the separately published `kit.json` before extracting/running.
