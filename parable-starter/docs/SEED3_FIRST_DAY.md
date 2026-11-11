# Seed3 first day — PAR-009

**Docs/source verified; hardware and Mac execution untested.** Use a reviewed
`seed3-first-boot-kit` artifact (30 days); retain manifest/source SHA/checksums.
All three images: **BOOT_NONE, internal flash 0x08000000, 128 KiB**. No QSPI loader.

## Prepare the 2013 Intel iMac / macOS 10.13

Use a **USB-A-to-USB-C data cable**, directly to Seed3. Firefox lacks native
WebUSB; don't assume current Chrome/Homebrew works on High Sierra. Arduino's
checked Intel `dfu-util` declares macOS **10.12 minimum**, system libraries only:
**metadata compatible, execution/USB on 10.13 untested**.

In Terminal (no installer, Homebrew, admin changes or guessed library paths):

```sh
mkdir -p "$HOME/Parable-PAR009"
cd "$HOME/Parable-PAR009"
curl -fL https://downloads.arduino.cc/tools/dfu-util-0.11-arduino5-darwin_amd64.tar.gz -o dfu-util.tar.gz
printf '%s  %s\n' 9e576c6e44f54b1e921a43ea77bcc08ec99e0e4e0905f4b9acf9ab2c979f0a22 dfu-util.tar.gz | shasum -a 256 -c -
tar -xzf dfu-util.tar.gz
./darwin_amd64/dfu-util --version
unzip "$HOME/Downloads/seed3-first-boot-kit.zip" -d kit
cd kit
shasum -a 256 -c SHA256SUMS
```

Require successful `--version` before flashing. Record checksum/dyld/security/TLS
errors; don't disable system security. **Fallback dependency:** access to native
Ubuntu **24.04 x86_64** with USB (not this cloud/untested VM). Run
`sudo apt-get update`, `sudo apt-get install dfu-util screen`, `dfu-util --version`.
Extract/check the same kit with `sha256sum -c SHA256SUMS`; use `sudo dfu-util`
below with local file paths. No ARM compiler required to flash supplied bins.

## Ordered procedure

1. **Module alone:** carrier, external power, guitar/amp disconnected. Inspect
   module/labels; use USB-C power/data only. No header wiring or analog test yet.
2. **Enter STM32 system ROM DFU:** hold **BOOT**, press **RESET**, release **RESET**,
   then release **BOOT**. Run `cd "$HOME/Parable-PAR009"`, then
   `./darwin_amd64/dfu-util -l`. Expect a ROM DFU device normally identified as
   `0483:df11`, alternate 0 **Internal Flash / 0x08000000**. Save output; VID/PID
   alone isn't ROM identification. Missing? Repeat buttons, try another data cable/
   direct port, inspect System Information → USB. Unexpected region/device? Stop.
3. **LED/recovery image:** from that parent folder:

   ```sh
   ./darwin_amd64/dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D kit/led_recovery/parable_seed3_led.bin
   ```

   Expect **one short LED pulse per 3 seconds**, no codec/external-memory/DSP init or USB CDC.
   DFU disappears after `:leave`; press RESET once if needed. Repeat step 2 to
   verify ROM recovery. Failed/hung image: use **BOOT-held RESET**, reflash LED.
   Raw bins may warn about a DFU suffix; success requires completed download
   **and observed heartbeat**. Save other errors.
4. **Carrier gates, module absent and USB unplugged:** require the selected #26
   schematic/revision, orientation and testpoints. Meter named **9 V/5 V/3.3 V**
   rails/polarity against that design. Module-derived 3.3 V makes its no-module
   test **blocked pending #26's method**. Never invent pins/testpoints, back-power
   3.3 V or put raw guitar/9 V on GPIO. Unverified rail means stop.
5. **MONO BYPASS:** standalone USB-only, repeat ROM DFU, then:

   ```sh
   ./darwin_amd64/dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D kit/mono_bypass/parable_seed3_bypass.bin
   ```

   Power off; install with confirmed orientation. **One approved power source**,
   amp volume down. Guitar enters only the verified carrier instrument jack;
   approved output goes to amp. Raise gently; record dry behavior/noise/dropouts/
   levels. Input0 is copied to outputs0/1: **mono**. Two LED pulses indicate firmware
   readiness, not proven audio. Unstable? Return to standalone LED recovery.
6. **RAT COMPOSITE:** power off; return to standalone USB/ROM DFU:

   ```sh
   ./darwin_amd64/dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D kit/rat_composite/parable_seed3_rat_composite.bin
   ```

   Same installation/power rules. Three pulses identify
   **Gain1.8 → Rat .35/.5/.7 → Lowpass1800 Hz, trim .8**, master Volume1: a build-only
   smoke patch, not a saved/editable preset or calibrated I/O. Amp down before
   RESET; record short guitar audio in the first ~30 s. Retain dry comparison/settings.
7. **CPU log:** first **30,000 callbacks** freeze results (~30 s at configured
   48 kHz/48 frames); audio continues. USB CDC then starts, repeating text once/sec.
   USB-only logs use **unconnected input**, not a guitar performance test.
   **Carrier+USB power coexistence/isolation needs #26 approval**. Otherwise
   guitar+USB measurement is blocked: do only standalone USB capture.

   On the Mac, `ls /dev/cu.usbmodem*`, select the actual new device, then
   `screen -L /dev/cu.usbmodemACTUAL_NAME 115200`. Replace `ACTUAL_NAME` with the
   listed path; `screenlog.0` captures text. Exit Ctrl-A, then Ctrl-\, then y.
   Linux uses `/dev/ttyACM*` with appropriate permissions. Missing port? Save
   LED/USB listings, use ROM recovery; USB/driver retrieval remains unproven.

## Read the result and retain rollback

LED codes: **1 LED-only; 2 bypass; 3 RAT; 4 wrong board; 5 config/report error;
6 native load failure; 7 SDK start failure; 8 no capture completion in 60 s;
9 counter unavailable (audio may continue)**. Solid LED is initializing. Some
SDK clock/codec/USB faults can hang before a code; ROM recovery remains the failsafe.
Pinned SDK start does not propagate every DMA failure; a ready LED is insufficient.

Logs supply RCC core Hz, configured milliHz/frames, estimated deadline, max valid
callback-body cycles/ppm, invalid/deadline counts and stopped-load time (both RAT
preparations). Ppm/10,000=percent. Require counter available, **30,000 valid**
intervals, no invalid/frame mismatches. Excludes bookkeeping tail, return, SDK ISR
and later USB. Intervals must be <2^31 cycles; multi-wrap hangs aren't resolvable.
**Observed max isn't worst possible, p99 or latency**; postcapture USB glitches
are separate. USB class allocation/IRQ/heap robustness still needs physical tests.

**Evidence checklist/template:** source/bin hashes/power arrangement: PENDING;
DFU/LED/rails/orientation: PENDING; rate/frames/core/deadline/max/ppm/counters/startup:
PENDING; noise/dropouts/recording: PENDING; static RAM/frames: `manifest.json`;
heap/stack high-water: UNMEASURED. Physical deadline/headroom/musical gates remain
pending. Reset to repeat capture. Preserve original
`rollback/parable_seed3_minimal.bin` bypass (no logging/hooks) and LED image.
To restore original bypass, enter ROM DFU and use the same flash command with
`-D kit/rollback/parable_seed3_minimal.bin`. This legacy rollback has no heartbeat
or CDC; use LED recovery for the USB-only gate.

Sources: [official ROM procedure](https://docs.daisy.audio/troubleshooting/),
[Seed3 USB-C](https://docs.daisy.audio/hardware/Seed3/),
[ROM vs Daisy loader](https://docs.daisy.audio/tutorials/_a7_Getting-Started-Daisy-Bootloader/),
[native utility/Arduino manifest audit](PAR009_GATE0.md). The web programmer's
**File Upload** on a supported browser is an alternative; its **Bootloader tab,
`make program-boot`, SRAM/QSPI binaries and mass-erase are not this procedure**.
Internal-flash programming replaces any installed Daisy software loader; STM32
system ROM itself is not erased. Do not infer software-loader recovery UX here.
The optional Daisy software loader uses a **breathing-LED timeout**, extended by
**BOOT alone**, for QSPI-stored apps. That is different from **BOOT-held RESET
into ROM** used throughout this guide.
