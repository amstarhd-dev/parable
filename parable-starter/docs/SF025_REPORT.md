# SF025 — Virgo Tiled Room

**[Download the listening ZIP](../samples/tiled_room/SF025-listening.zip)** ·
[healthy](../samples/tiled_room/healthy_matched.wav) ·
[fault](../samples/tiled_room/fault_matched.wav) ·
[existing Room A](../samples/tiled_room/comparison_roomA_matched.wav).
Mono/stereo, raw/matched, wet-only, impulses and transient diagnostics are included.
Principal auditions retain **65% dry guitar /35% wet**. The processor itself permits
0–100% wet; no universal mix policy is imposed.

Healthy-only milestone c5a072678bc640a43a154effcbb12e8502cefada was delivered in
PR80 BEFORE fault implementation. Founder response: **“Promising—continue.”**
That healthy raw waveform is still byte-identical. This is provisional healthy
feedback, not PCM70 equivalence or approval of the final fault. **Leave unmerged.**

## Decision and prior art

[Revised assignment](SF025_ASSIGNMENT.md), [Gate0](SF025_GATE0.md),
[ADR025](adr/ADR-025-virgo-tiled-room.md). Earlier treated-room/flutter brief was
superseded by this Tiled Room/diffuser-ringing brief. Current-main starting SHA
767a3c402b972fc20df8aea0729adf794f4b7a38; no other PR edited/merged. Read AGENTS,
Constitution, README/status, ADR001 and Soundforge methodology before code.
Baseline core/audio/shared utilities/registry **5/5**,1.06s. No root AGENTS exists.

Mandatory primary source: [Sean Costello,2009](https://valhalladsp.com/2009/12/22/eos-tips-and-tricks-tiled-room/),
[author's accessible original mirror](https://valhalladsp.wordpress.com/2009/12/22/eos-tips-and-tricks-tiled-room/).
Manufacturer documentation: PCM70 Rev2 owner’s manual, Rich Chamber4.2 and
chapters3/4 (especially4-5/4-6), read through [manual mirror](https://manualzz.com/doc/11639692/lexicon-pcm-70-digital-effects-processor-owner-s-manual).
Official Lexicon download page returned403; not bypassed. Costello describes a
plate-family recreation, not a published PCM70 circuit or full algorithm.
The manual emphasizes dense, articulate small sizes and independent early taps.

| Published clue | This independent candidate |
|---|---|
| ≈4ms predelay; compact apparent size | 4ms explicit delay; fixed dimensionless size.30; measured first return5.35ms |
| Rapid initial density, restrained tail | Original four input allpasses and complete signed stereo tank taps; default T20-extrapolated RT60≈.583s |
| Bright but frequency-dependent decay | Input LP10k/tank LP6.5k; no claim these map exactly to Eos/PCM70 controls |
| Rich Chamber has no modulation | LFO removed entirely; all lengths fixed |
| Separate running/stopped behavior and user early reflections | **Not reproduced**; fixed linear decay and source tank readout |

Investigated actual MIT **Mike Jarmy dsp-lib** and **Signalsmith Basics** complete
implementations, plus GPL Dragonfly/Freeverb3, John Williamson’s C Dattorro and
Mutable Rings/Clouds. Select Jarmy a26e67c8c9a12038cc51fdec5f7bed3941ce465b:
complete figure8/input diffusion/damping/tap signal flow, small static native port.
Signalsmith’s complete8line FDN was configured/runnable with short size18ms,
rt20.207s and detune0 in an exploratory8s pass (.173µs/frame); that pass differs
in duration/input calibration and is **not** a controlled quality/CPU ranking.
Cancer CloudSeed and Leo kCathedral are larger/different networks; not modified.
No unmerged Taurus mechanics or reverb core was merged into this branch.

Borrowed: Jarmy scalar algorithm, delay interpolation, coefficients and tap signs;
original headers/notices retained verbatim. Original reference test changes only
modulation depth to0 in a generated build header. Native does not import its LFO
or FastMath runtime. Independently implemented: bounded caller storage/control
validation, compact tuning, stereo/mono adapter, one coefficient fault and full
complex-domain validation. Not a newly invented generic FDN/framework.
[Exact pins/rights](../third_party/tiled_room/provenance.json), MIT notice retained.
No proprietary Lexicon/Eos code, firmware, commercial IR or plugin imported.
Dragonfly/Freeverb3 remains GPL audit-only; registry license policy unchanged.

Located publisher-hosted [AMAZONA PCM70 drum examples](https://www.amazona.de/zeitmaschine-lexicon-pcm-70-digitaler-effektprozessor-1985/)
and public demonstrations [xiDudA83mfQ](https://www.youtube.com/watch?v=xiDudA83mfQ),
[yQTvOa-xSEg](https://www.youtube.com/watch?v=yQTvOa-xSEg) as reference listening
sources. No redistribution right, same-source A/B or agent auditory judgment
inferred from their availability. No PCM70 IR/copyrighted demo copied into Git.
**A controlled PCM70 comparison has not been performed.**

## One same-network failure

Only the existing **right-tank second-allpass recirculation coefficient** changes:
normal clamp(g+.15,.25,.5), default.5 → **.94**. The same coefficient remains tied
in its original feedforward expression. Same13 delay histories, source, geometry,
filters, cross-loop gain, controls and readout; no added oscillator, resonator,
extra delay line, sample, gate or output layer. This is an algorithmic attenuation
failure, not a claim that a real tile panel was physically modeled.

For an integer delay the existing recurrence is W=X/(1−a*z⁻ᴰ),
Y=(z⁻ᴰ−a)W. Greater a reduces loss within that recirculation history; original
output taps already observe it. Allpass|a|<1, interpolated delay is contractive,
and unchanged cross-loop gains<1: bounded linear ringing, not self-generated
excitation. Weak signals ring proportionally less; no invented loudness threshold.
This differs structurally from Leo’s H→I scattering-plane focusing; whether the
musical personalities are sufficiently distinct remains for listening.

Independent full z-transform solution includes fractional delays, filters,
cross-feedback, diffuser internal histories and signed stereo taps. Against native
six-second f32 impulse spectra: max complex error **4.14e-7 healthy /1.44e-4 fault**,
relative L2 error1.22e-7/1.18e-5. Six-second truncation/native coefficient rounding
contribute residuals; this is digital-network evidence, not physical acoustics.
[Reference equations/results](../samples/tiled_room/reference.json).

Measured T20-extrapolated RT60 **.583→2.812s**. Impulse energy300–1000ms rises
**17.81dB**; one resolved modal point≈374Hz rises **19.34dB**, versus only.7LU
whole-guitar raw level change. This supports selective persistence, not merely a
louder wet return. Healthy density rises rapidly over10–80ms. Numerical changes
do not establish that the result sounds ceramic, appealing or authentically Lexicon.

## Recordings and fair comparison

Same licensed Guitar-TECHS Player3 take03 late20s excerpt as SF023/024, source
9.28–29.28s, channel0/24→16, no resample/time stretch/artificial notes. Same .2
software input peak,3s tail,23s total. [Authors/license/change notice](../samples/tiled_room/AUDIO_NOTICE.md)
and source receipt retained. Healthy/fault principal mix has identical.65dry/.35wet,
raw output gain1. Matched mono and stereo copies each remeasure **−23LUFS** using
external FFmpeg ebur128; constant scalar gains only. No loudnorm/compressor/limiter.
Scaling starts from rawPCM16 exports, so their quantization remains. Matched mono
true peaks−6.2/−6.4dBTP; actual raw peaks.14059/.15010. Wet solos use the fullmix
gain; paired transient listen files use one shared healthy gain. No normalization
is inserted into native DSP to manufacture a fault.

Comparison is the **exact unchanged SF020 Room A**, aperture0, nominal.65s;
measured T20-extrapolated decay≈.558s, close to Tiled.583s. One offline scalar
**2.6804×** equalizes its wet RMS to Tiled before the same.65dry/.35wet mix;
original unscaled wet is also published. Existing algorithm is not retuned/rewritten.
This comparison tests character at comparable decay/strength, not universal superiority.
The copied prior core/header and Signalsmith notice have hashes/origin in provenance;
they are host comparison-only, never in the new portable library/ARM image.

Actual mono files equal the arithmetic average of raw stereo within one PCM16
LSB; measured wet stereo correlation/power changes are in matching.json. Mono
averaging retains intentional phase interactions, not a promise of identical
stereo loudness/space. Input is mono with synthetic stereo readout, not true-stereo
input processing. Full20s repeat reproduces **13 native WAVs +2 float impulses**
byte-for-byte; CLI separately verifies two complete short renders.

## Resource evidence

Final GNU14.2/O3/EPYC9V74 host pass,1104000frames/mode, actual full processor plus
four capture stores: **healthy.06561 /fault.06392µs/frame**. First healthy-only
pass.06780; repeat receipts retained. Clock/scheduler uncontrolled, not WCET,
MCU cycles or a claim fault runs faster. Preparation/I/O excluded. Fixed work,
no iterative solver/modulation; callback allocation/free/lock/I/O guards pass.

Host metadata344B plus caller floats: **44588B at44.1k /48532B at48k /96916B at96k**.
At48k total48876B; no hidden delay heap. Caller storage lives through stopped-only
reset/reprepare and single-owner processing. Native input domain finite±1; invalid
samples return0 without advancing history. Decay control.25–1.2s, wet0–1.
No hidden output limiter: linear wet returns retain headroom, so target gain and
output clipping behavior still need physical evaluation.

Actual pinned xPack14.2.1/libDaisy1289a000 + exact submodules: newly compiled
native/probe/startup and **full standalone audio-image link** in official unchanged
BOOT_NONE128KiB layout. Reused exact prior SDK archive, SHA and clean source
identity recorded; SDK was not redundantly rebuilt. No flash enlargement,
bootloader migration, SDK edit, production firmware change or automatic flashing.

- **Flash load79472B**: vectors664, .text77436, .exidx8, init/fini40/4, data initializer1320.
- **RAM**: .data1320 /.bss60868 AXI SRAM; DMA/D2 .sram1_bss16384; backup12;
  .dtcmram_bss0, **.sdram_bss0**. These include SDK and application, not call-stack high-water.
- Actual ARM room332B + exact48k history48532B = **48864B**; no SDRAM for this image.
- Native object **2588B text,0data/BSS**. Audio functions call only native helpers;
  no audio allocation/lock/I/O/libm/double instructions. Preparation alone uses libm.
- Known callback48 + process16 + allpass16 + tap4 = **84B static frame chain** before
  SDK dispatch/interrupt context; not measured high-water or a deadline proof.

[Actual link/section/compiler/stack/disassembly/map receipts](data/sf025/arm-report.json).
No physical Seed3 boot/audio/codec/headroom/cycle/latency proof. This is a credible
resource prospect, **not hardware admission** or an integrated patch-engine voice.

## Tests and reproduction

Baseline5/5. Final focused **Release7/7**, **Debug2/2**, **poisoned ASan+UBSan2/2**;
139 native checks. Source parity at all3rates/3decay endpoints, max error1.49e-8
(default.62 recipe exact0); independent complex network; audible-history reset,
invalid/history preservation, sine/tail/DC stress, callback restrictions, actual
CLI/provenance and deterministic files. Existing core/audio/shared utilities/registry
also pass; inventory expected count13→14 for one added specimen. No old model
threshold/golden/reference changed, no massive unchanged circuit/SPICE campaigns.
**452 pre-existing native/header/firmware/fixture files byte-identical to main.**

Clean checkout C++17, CMake>=3.15, Python>=3.11 for NumPy2.3.5 analysis:

```bash
cmake -S parable-starter -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release --target parable_tiled_room_test parable_tiled_room_lab parable_core_test parable_audio_safety_test parable_soundforge_test -j2
ctest --test-dir build/Release --output-on-failure -R '^(tiled_room_|core_validation|audio_safety|soundforge_)'
cmake -S parable-starter -B build/Debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG='-O0 -g1'
cmake --build build/Debug --target parable_tiled_room_test parable_tiled_room_lab -j2
ctest --test-dir build/Debug --output-on-failure -R '^tiled_room_'
cmake -S parable-starter -B build/Sanitized -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-O1 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build build/Sanitized --target parable_tiled_room_test parable_tiled_room_lab -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:max_malloc_fill_size=100000000 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/Sanitized --output-on-failure -R '^tiled_room_'
./build/Release/parable_tiled_room_lab --input parable-starter/samples/tiled_room/take03-late20s.wav --output build/tiled-auditions
# External analysis: NumPy2.3.5 and FFmpeg ebur128 (local7.1.5).
python3 parable-starter/tools/tiled_room_reference.py --directory build/tiled-auditions
cp parable-starter/samples/tiled_room/AUDIO_NOTICE.md parable-starter/samples/tiled_room/README.md build/tiled-auditions/
python3 parable-starter/tools/tiled_room_evidence.py --directory build/tiled-auditions
python3 parable-starter/tools/check_tiled_room_arm.py --compiler PATH/arm-none-eabi-g++ --libdaisy PATH/EXACT-PINNED-libDaisy --output build/tiled-arm
```

The established firmware/build.py recipe obtains/builds the pinned toolchain/SDK
on a fresh Linux machine; no need to flash. ARM script checks exact source/submodule/
compiler pins and clean SDK, then links a separate48k build-only specimen.
Local roots `/workspace/work/sf025-{release,debug,sanitized}`, CMake/CTest3.31.6
via `/workspace/work/baseline-tools/bin`; offline pinned JSON override
`-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src`.
ARM compiler `/workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++`,
SDK `/workspace/work/par005-fresh-cache/libDaisy`. Python3.12.14, NumPy2.3.5,
FFmpeg7.1.5; source/tool identities/logs beside report. Build/version differences
can change last bits/meter rounding; no cross-platform bit-identity promise.

Changed files: one native header/core; bounded lab/meter/reference/ARM probe tools;
focused tests/CMake/CI and one registry entry/count; pinned original source/license
and exact old comparison core; this report/ADR/Gate0/status, licensed audio/ZIP/data.
No Engine/canonical/activation/storage/firmware/previous reverb/astrology edits.
New CI job is additive; remote results reported separately in PR, not inferred
from local success. ZIP and WAVs are committed independently of CI expiry.

**Open acceptance:** final ceramic fault approval; musical distinction from all
previous voices; authentic PCM70 identity/controlled original comparison; physical
Seed3 operation/performance/high-water and analog headroom. No live parameter
updates/clickless reset or Engine admission. Healthy was called promising, not
ratified as an exact historical reproduction.

**Next smallest step:** listen to matched healthy/fault and common-gain transients,
then wet solos; compare short-room character with Room A. If retained, measure
this standalone48k image’s cycles/headroom/audio on actual Seed3. No larger
architecture or additional effect needed. Leave PR unmerged for independent review.
