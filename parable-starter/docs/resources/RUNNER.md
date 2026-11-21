# Seed3 automated benchmark runner — experimental

**BLOCKED: v1.1 physical identity verification fails at ROM SET_ADDRESS. Do not
retry its probe/batch as an operational recipe.** See [identity investigation and
replacement design](IDENTITY_BLOCKER.md). The original steps below are retained
for review; no replacement identity backend or working kit is claimed.

**Software tests and 15 ARM links pass. Physical handoff and the one-button,
15-image acceptance test are NOT tested. Batch remains gated by a successful
physical probe.** macOS 10.13 execution is also untested. This is an unmerged
proposal stacked on PR #83; neither PR should merge automatically.

The runner uses Python **3.6 or later**, standard library only: no Homebrew,
pyserial or fixed serial-device filename. It discovers CDC via IORegistry,
validates silicon/firmware identities and saves each report before authorizing
the next reset. All images remain **foreground kernel** benchmarks with audio
stopped. Their cycle fraction is **not real-time callback utilization**.

## v1.1 descriptor correction

The founder's physical probe reported exactly
`@Internal Flash /0x08000000/16*128Kg`. The previous one-sector-only validation
was incorrect. This shared ROM map advertises sixteen 128 KiB sectors; it is
**not proof of 2 MiB available flash**. The runner now recognizes that descriptor
and the legacy `1*128Kg` / `01*128Kg` forms, while still requiring alt0, interface0,
configuration1, base `0x08000000`, the selected serial/path and independent UID,
chip ID and **128 KiB factory flash-size** verification. Firmware remains bounded
to 128 KiB by both the image verifier and original linker script.

This is a host parser correction. All 15 BIN/ELF/map payloads, firmware identities
and the firmware manifest SHA below are unchanged. `runner-manifest.json` records
the new runner version/hash; `SHA256SUMS` and external `kit.json` are regenerated.
Use this freshly extracted ZIP and new output folders. The changed runner hash
requires a new successful probe receipt before batch can run. ROM enumeration was
reported by the founder; **identity uploads, software ROM handoff and unattended
batch acceptance remain unproven**.

## Download and prepare

Download the [versioned runner ZIP](https://github.com/amstarhd-dev/parable/raw/refs/heads/engineering/seed3-automated-runner/parable-starter/docs/resources/runner/seed3-runner-v1.1-f2c156bb74dd-764d589fcc68.zip). Extract it to a new folder. It
contains 15 uniquely named BINs, matching ELF.gz/map/static-frame/link evidence,
manifest, reset-hook disassembly, licenses, source-preservation receipts, runner
and this guide. Original PR #83 binaries/IDs are retained as references; modified
images have new IDs and explicit `parent_artifact_id` bindings. Never flash the
original kit and expect it to accept the new protocol.

Verify `SHA256SUMS` inside the ZIP with `shasum -a 256 -c SHA256SUMS`. Obtain the
**manifest SHA256 from the PR / checked-in runner evidence**, separately from the
downloaded files. Checksums establish consistency with that reviewed identity,
not an independent trust/signing service.

Download the same pinned Intel Mac dfu-util used in PAR-009:

```sh
curl -fL https://downloads.arduino.cc/tools/dfu-util-0.11-arduino5-darwin_amd64.tar.gz -o dfu-util.tar.gz
printf '%s  %s\n' 9e576c6e44f54b1e921a43ea77bcc08ec99e0e4e0905f4b9acf9ab2c979f0a22 dfu-util.tar.gz | shasum -a 256 -c -
tar -xzf dfu-util.tar.gz
./darwin_amd64/dfu-util --version
python3 --version
```

The runner verifies the executable SHA256
`a200781d74555c0d9c4b379fc7d5872deed6e13bf3988fc9d6313210090a8f43`
before any USB access. PAR-009 Mach-O inspection found deployment minimum 10.12
and system-library dependencies. That metadata is not proof of High Sierra USB
execution. Do not disable system security or substitute an unverified executable.

Close serial monitors, Daisy web tools and other USB flashers. Connect only this
Seed3 directly using a data cable. Leave audio disconnected for these original
synthetic foreground benchmarks. Disconnect downstream amplifiers while flashing.
Retain the PAR-009 bypass recovery kit if you intend to restore performance
firmware later. No bootloader installation or option-byte edit is needed.

In the extracted runner kit folder, set the reviewed manifest hash:

```sh
MANIFEST_SHA=f2c156bb74dd792b273e7f5a33d66ca75c7d676b6a5a7bdd534e723ada2f5fe3
python3 seed3_runner.py verify --manifest artifacts.json --manifest-sha256 "$MANIFEST_SHA"
```

`verify` performs no USB access. It checks all 15 profiles, hashes, configuration
identities, source closures, official linker pin, internal-flash bounds and reset
vectors. It rejects BOOT_SRAM/QSPI and incomplete/different kits.

## Separate physical transition validation — do this first

1. Hold BOOT, press and release RESET, release BOOT. This is the **one initial
   manual ROM entry**. Ordinary BOOT/RESET recovery remains available afterward.
2. Run `./darwin_amd64/dfu-util -l`. Copy the `serial="..."` value for the one
   `0483:df11` device. The observed Seed3 alt0 name is `@Internal Flash /0x08000000/16*128Kg`;
   the advertised sector count does not establish physical flash capacity.
3. Set the ROM serial and run the probe below. It first uploads read-only silicon
   identity words. If any read/layout/identity check fails, **it will not flash**.
   Send the full diagnostic for review; do not weaken the checks.

```sh
ROM_SERIAL=REPLACE_WITH_THE_REPORTED_ROM_SERIAL
python3 seed3_runner.py probe --manifest artifacts.json --manifest-sha256 "$MANIFEST_SHA" \
  --dfu-util ./darwin_amd64/dfu-util --serial "$ROM_SERIAL" \
  --output probe-002 --authorize-dfu-reset
```

The probe flashes the labeled **rat_1x_cost_only** image (a control-path test,
not an audio-quality candidate), waits for its complete frozen report, saves and
syncs it, checks CDC identity, sends the artifact-bound reset command and verifies
that **the same ROM device and silicon return without touching BOOT/RESET**.
Success produces `probe-002/transition-receipt.json`, leaving the board in DFU.
Keep the entire probe folder. No receipt is produced on a failed transition.

**If the board stays in CDC, goes missing, or identity reads fail: stop.** The
physical gate has failed, not passed. The runner reports a diagnostic and makes
no further flash attempt. Use BOOT/RESET to recover, retain logs, and request a
reviewed correction. Do not manually create a transition receipt. A different
kit, runner, tool, board or USB port requires a fresh probe.

## Unattended batch after a successful probe

Leave the same board/cable/port in the returned DFU state. No further button
operation is needed when the handoff works:

```sh
python3 seed3_runner.py batch --manifest artifacts.json --manifest-sha256 "$MANIFEST_SHA" \
  --dfu-util ./darwin_amd64/dfu-util --serial "$ROM_SERIAL" \
  --transition-receipt probe-002/transition-receipt.json \
  --output batch-002 --authorize-dfu-reset
```

The default benchmark/CDC wait is bounded at 900 seconds per image; adjust with
`--report-timeout 30..3600` if needed. Enumeration has separate bounded retries.
A flash error is fatal and is never blindly retried. Existing output folders are
rejected. After a failure, use a new output folder only after reviewing its cause;
there is no speculative mid-batch resume. Never swap boards during a run.

Each image produces raw `.txt`, silicon/control `.control.json`, and verified
return `.transition.json` files. `results.csv`, `results.json`, `results.md` update
as reports arrive. Numerical or budget screen failures are retained and do not
silently turn into admission. Malformed, mismatched or incomplete reports stop
the run. `full_15_image_acceptance` stays false until the final DFU return is
verified; **`batch-complete.json` is written only after all 15 reports and all 15
returns**. The board ends in ROM DFU, ready for a separately chosen recovery image.

To establish the physical acceptance criterion, retain the probe and batch
folders and confirm that no BOOT/RESET button presses or cable changes occurred
between initial entry and completion. This software cannot attest human actions.
No acceptance evidence is included in this PR.

## Control protocol

Report body is unchanged: `PARABLE_RESOURCE_V1 ...` through
`END_RESOURCE_CAPTURE`, repeating until HELLO. Existing report import/comparison
tools remain authoritative. Transfer completion unlocks the parser; receiving
bytes or enqueuing a transmission alone does not.

```text
host: HELLO <32 lowercase hex nonce>\n
board: PARABLE_CONTROL_V1 artifact_id=<64hex> nonce=<nonce> uid=<24hex> chip_id=<decimal> flash_kib=128 board=seed3 handoff=reset-systeminit-v1\n
host: ENTER_DFU <64hex artifact-id> <same nonce>\n
board: DFU_AUTHORIZED <same nonce>\n
```

Commands are strict LF-terminated ASCII, bounded to 127 bytes. Unknown, partial,
oversized, stale-nonce or wrong-artifact commands cannot reset the board. The
USB IRQ only copies into a 256-byte queue. Complete report and authorization ACK
transfers must finish before reset. Control, USB, formatting and reset all occur
after measurement/audio stop. No callback I/O/allocation/locks are added.

## Reproduce and inspect

From the repository root, with exact dependencies from firmware/dependencies.json:

```sh
python3 -m unittest discover -s parable-starter/tests -p test_resources.py -v
python3 -m unittest discover -s parable-starter/tests -p test_seed3_runner.py -v
python3 parable-starter/tools/resources/firmware.py --usb-control \
  --compiler "$ARM_GXX" --libdaisy "$LIBDAISY" --output build/runner --fetch \
  --only rat rat_fir97 rat_fir97_fp32 rat_1x_cost_only cmos germanium muff ts808 klon blender fz1 tape_saturation spring leo_hall chain_scorpio_gemini_rooms_0
python3 parable-starter/tools/resources/audit_handoff.py --kit build/runner --objdump "$ARM_OBJDUMP"
python3 parable-starter/tools/resources/package_runner.py --build build/runner --output build/runner-package
```

`--usb-control` restricts only the new handoff to BOOT_NONE. The existing
uncontrolled report-only generator and its other layout options remain available.
Production firmware/build.py and all DSP sources remain unchanged. Exact compiler
and SDK archive hashes are recorded; rebuilt dependencies/debug paths can change
artifact identities. A changed artifact needs its own manifest and probe, rather
than reusing a previous ID or physical measurement.

The post-edit focused suites pass **26 tests** (8 existing + 18 runner/package tests).
The actual command parser and existing DSP chain also run under ASan/UBSan.
No unrelated DSP/SPICE/audition campaign was rerun.

See [Gate 0](RUNNER_GATE0.md), checked-in `runner/validation.json` and the kit's
`source-preservation.json` / `handoff-audit.json`. Passing source/ELF inspections
proves neither actual ROM access nor correct analog/audio/hardware timing.

## Linked resource evidence

All execute from internal flash at `0x08000000`, with the official 128 KiB limit.
Static internal RAM totals data/BSS/mailbox/DMA/backup sections; they exclude
runtime USB heap and stack high-water. SDRAM values are linker reservations,
not proof of physical external-memory operation. Every image adds 32 mailbox
bytes; all sections and addresses are in the manifest/ELF.

| Profile | BIN bytes | Static internal RAM bytes | Static SDRAM bytes |
| --- | ---: | ---: | ---: |
| rat | 106888 | 58840 | 0 |
| rat_fir97 | 106904 | 57976 | 0 |
| rat_fir97_fp32 | 106728 | 57904 | 0 |
| rat_1x_cost_only | 106904 | 58840 | 0 |
| cmos | 97040 | 50216 | 0 |
| germanium | 99704 | 50568 | 0 |
| muff | 99720 | 56920 | 0 |
| ts808 | 109744 | 56192 | 0 |
| klon | 102520 | 59896 | 0 |
| blender | 111904 | 67968 | 0 |
| fz1 | 100440 | 53224 | 0 |
| tape_saturation | 103608 | 59968 | 0 |
| spring | 98128 | 66824 | 0 |
| leo_hall | 109192 | 48080 | 475480 |
| chain_scorpio_gemini_rooms_0 | 112120 | 62056 | 866304 |
