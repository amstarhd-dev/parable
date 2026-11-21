# ROM DFU identity gate — blocked, not a successful physical probe

A follow-on [separate RAM-only test](RAM_IDENTITY.md) is now implemented and
packaged for physical validation. It does not integrate with or unlock the full
runner. The investigation below still applies to the unsupported v1.1 backend.

The founder's v1.1 log reports USB DFU 011a, transfer size1024, alt0 Internal
Flash, initial state2/dfuIDLE and status0, followed by
`SET_ADDRESS not correctly executed`. Enumeration succeeded. Identity verification,
software ROM handoff and unattended acceptance did not succeed.

## Failing operation and evidence limits

The implementation is `Mac.identity()` in seed3_runner.py (not a class named
Seed3Runner). It issues these sequential commands and stops on the first error:

| Order | Address / bytes | Intended identity | Documented ROM eligibility |
| --- | --- | --- | --- |
| 1 | 0x1FF1E800 /12 | 96-bit UID | Outside listed H74/H75 system-memory range |
| 2 | 0x5C001000 /4 | DBGMCU_IDCODE | Peripheral register; no documented DFU read route |
| 3 | 0x1FF1E880 /2 | factory flash size | Outside listed H74/H75 system-memory range |

For an unmodified v1.1 run, operation1 is therefore the first failing request;
operations2/3 are not reached. The two pasted blocks repeat the same failure,
without an address/command line. They do not demonstrate two different reads.
Initial dfuIDLE/status0 argues against a pre-existing error state. The message
identifies SET_ADDRESS completion checking, before UPLOAD data transfer. It does
not supply the final bStatus/bState or a USB transaction trace: **errTARGET is the
expected protocol explanation, not a status value actually captured here**.
Retain the full STOP command line and, if needed, verbose/USB trace to confirm
address and returned status independently of code-order inference. Do not repeat
unsupported reads merely to obtain that trace.

## Primary-source findings

[ST AN2606 Rev71](https://www.st.com/resource/en/application_note/an2606-.pdf),
section102/table239, printed pages547–552, lists H74/H75 PID0x450, BL ID0x92,
system memory **0x1FF00000–0x1FF1E7FF** and available RAM
0x20004100–0x2001FFFF /0x24005000–0x2407FFFF. UID starts immediately beyond
that system-memory endpoint; flash size is farther beyond it. CPU-readable
register definitions in CMSIS do not grant ROM-loader access. Individual ROM
versions still require physical characterization; none is inferred from the USB
DFU bcdDFU value011a, which is not the bootloader software revision.

[ST AN3156 Rev19](https://www.st.com/resource/en/application_note/an3156-usb-dfu-protocol-used-in-the-stm32-bootloader-stmicroelectronics.pdf),
sections4.1/5.2, limits reads/address pointers to valid device memory areas and
refers to AN2606. SET_ADDRESS executes through GETSTATUS; unsupported addresses
can enter an error state. No documented peripheral-read operation covers DBGMCU.
Section5.5 documents loading and leaving DFU into RAM, with vector-table and USB
handoff requirements. The pending-USB/core-reset caveat must be resolved for an
actual RAM executable; a generic BOOT_SRAM image is not sufficient evidence.

`:force` bypasses dfu-util's host memory-map eligibility check. It neither enlarges
the MCU's allowed ranges nor grants a privileged ROM operation. The previous
raw-address identity design is **not a supported basis for admission**. Changing
only -a, transfer size, or switching USB frontends cannot establish otherwise.
No offset/block-number tricks to evade address validation are proposed.

## Supported replacement design, requiring another implementation increment

A **volatile RAM identity probe** is the closest candidate that preserves both
DFU flashing and no-flash-before-verification:

1. Build a small, hash-pinned RAM-only executable with its vector/code/data/stack
   wholly within the documented available RAM ranges, excluding ROM workspace.
   No flash/option-byte/backup writes, production DSP or audio initialization.
2. Validate its manifest and exact bounded RAM addresses before a ROM RAM
   download. Read back RAM and verify bytes. No firmware flash occurs here.
3. Execute it using the documented RAM leave operation. Audit startup, VTOR,
   pending USB interrupts, clocks/cache and the AN3156 core-reset caveat first.
   This is a **design gate**, not an implemented or proven recipe.
4. Have native CPU code read all96 UID bits, chip ID and factory size and report
   them with a host nonce and probe identity over CDC. Bind this result to the
   selected USB path, then verify return to that same ROM serial/path. Reject
   ambiguous boards; bind every later firmware CDC report to the full UID.
5. Only after verification may a <=128KiB benchmark image be written. Retain the
   separate software-to-ROM transition gate. Re-enroll after any disconnect,
   replacement board or ambiguous enumeration. Document that normal USB identity
   is not adversarial cryptographic attestation.

These are documented RAM transport mechanisms, not physical Seed3 compatibility
proof. The follow-on RAM helper is implemented only as the separate physical test. It
keeps interrupts masked until a full system reset and starts no USB stack,
addressing the startup requirement in its design. Physical behavior is unproven. Do not send an ordinary flash-linked image to RAM or modify option bytes.

A second supported transport is read-only **ST-LINK/SWD** acquisition, followed
by verification and flashing of that same attached target via SWD. [ST UM2237 /
CLI reference](https://dev.st.com/stm32cube-docs/prog/2.23.0/en/docs/markup/CubeProg_Command_Lines.html)
documents SWD and memory reads. It avoids ROM's address whitelist. This requires
a probe/wiring, a reviewed target-binding backend and host-version compatibility
validation; it changes the dfu-util-only requirement and is not silently selected.
A loose manually edited receipt, USB serial alone, inferred2MiB descriptor,
preflashed identity firmware, or accepting missing values is not a replacement.

## Delivery status and next test

v1.1 remains blocked at identity verification and must not be treated as a usable
unattended kit. No corrected full-runner ZIP is published. A separately packaged RAM-only
instrument is available in the follow-on guide; the15 benchmark binaries and
old full-runner backend are unchanged. Added regressions inject failure at each identity request
and establish that no later read or flash download is issued. They simulate errors;
they do not prove ROM readability or physical handoff.

Next test: physically validate the standalone RAM download/readback and two
nonce-bound native identity captures with ROM return
**before any flash download**. If that cannot be supported, explicitly decide on
SWD transport. PR84 stays unmerged; original acceptance is unfulfilled.
