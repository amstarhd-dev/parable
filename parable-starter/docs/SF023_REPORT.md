# SF-023 — The Wallace experiment

**Listen:** [A serial](../samples/wallace/03_A_serial_lufs.wav),
[B parallel static](../samples/wallace/04_B_parallel_lufs.wav),
[C parallel dynamic](../samples/wallace/05_C_dynamic_lufs.wav), all−23LUFS.
[All nine comparisons, source and diagnostics](../samples/wallace/README.md).

The console produces the intended measurable changes: equal direct-path gain,
independent wet returns, bass/low-mid carving and up-to4dB ducking that recovers
between phrases. **Whether it is significantly more musical remains a listening
question.** No numerical result is presented as founder approval. This is one
performance/recipe, not a universal verdict in favor of parallel routing.

## Frozen implementations and baseline

Base main767a3c402b972fc20df8aea0729adf794f4b7a38. Read AGENTS, Constitution,
README/current status, Soundforge recipe, relevant main model reports and actual
APIs; [Gate0](SF023_GATE0.md), [full assignment](SF023_ASSIGNMENT.md),
[ADR023](adr/ADR-023-wallace-console.md) record scope and decision before code.
Before editing, focused current-main configure/build and core/audio-safety/shared
utilities/registry **5/5 passed**,1.00s. No new SPICE or redundant algorithm campaign.

Exactly four real processors: main `Muff`, `Rotary`, `Tape`; Hall copied
**byte-for-byte from PR77 commit21905d5ca171c1cb98d2e228dda33beac039ac4a**.
The four selected Hall source/header files and MIT notice have an enforced SHA
manifest. PR77 was not merged or modified, nor Cancer PR76. **452 existing native,
firmware and fixture files unchanged**. No original effect implementation/interface,
Engine, canonical patches, correspondence, storage, firmware or living modulation
change. No new general graph framework; the graph literally names these four.

Hall retains complete MIT Airwindows kCathedral via the exact native adapter;
original Chris Johnson notice retained. No VST SDK or GPL source import. Owner
explicitly authorized in-repository Parable reuse; global project license remains
unspecified. FFmpeg is an external measurement executable, not linked/imported DSP.

One real permitted source: full Guitar-TECHS Player3 take03, CC-BY4.0, unchanged
SHA84980b5468f3315171e4e546a1d38d54e4c368ccb5fee72c759e0915f346fad0.
[Attribution/change notice](../samples/wallace/AUDIO_NOTICE.md) names all authors.
Select9.28–29.28s from that same performance, channel0/24→16 nearest/ties-away,
no resample/fake gaps/time stretch/fade. This retains actual0.2–0.7s rhythmic gaps
and its last natural sustained decay. One common input gain gives0.2 software
peak, not physical pickup volts. Append9s zeros so every actual DSP tail develops.
The identical source/settings feed all comparisons. Full source/prepared input
remain in Git; input-receipt and wallace_input.py reproduce the excerpt.

## Actual architecture and fair gain convention

A: Muff -> Rotary35% -> Tape25% -> Hall20% -> mono.
B/C: same Muff -> one driven core plus separately fed **wet-only** Rotary/Tape/Hall.
Every send starts1; returns/core are .21/.20/.20/.39. This matches the expanded
serial control's zero-delay direct coefficient (.65×.75×.8=.39) and branch weights
(.35×.75×.8=.21,.25×.8=.20,.20). It does not manufacture a louder duplicate dry path.
The source models are nonlinear/stateful: the convention is not a mathematical
equivalence proof; routing changes later effects' content/excitation too.

Muff healthy Sustain.65/Tone.5/Volume.65 at original4x; same0.7 rotary speed command;
tape loop1.2s/delay.3s/feedback.32/drive2.4/record4500/replay6000/erase0;
hall healthy decay1/focusing0. In parallel, each effect's actual mix is100% wet;
no dry-subtraction proxy or generic substitute. The optional pre-Muff clean blend
is independent and defaults0; it is **not assumed to improve fuzz**.

B/C identical initial gains and return HPFs: Butterworth2pole120Hz tape /180Hz
hall. No other complementary EQ. C alone uses pre-Muff absolute input envelope:
10ms attack/250ms release, normalized thresholds.012/.05, max4dB. Gain interpolates
in amplitude from1 to10^(−depth/20), configurable0–6dB; no audio exp/log or step
gate. Duck only Tape/Hall, **not** core, clean or rotary. No mastering dynamics.
Renderer flags expose sends/returns/core/clean/HPFs/attack/release/depth.

08 bypasses only the two HPFs.09 is C without ducking, hence B; its required
recording reuses B bytes, and a separate actual-graph test confirms that equivalence
over nonzero input/tails. Full rendered B/C/08 native unweighted fuzz/rotary/tape/
hall stems are exactly identical. Changes therefore occur in the intended return
processing, not altered effects.06/07 isolate the weighted sum of three actual wet
returns. Unducked rotary can interact in phase with ducked returns; total wet
amplitude need not monotonically follow one return's gain.

All raw comparisons use **one identical output gain1**, no limiter or post clipping.
The existing four processors' safety/fallback counters are zero in normal renders.
Native intentional saturation remains part of each original model.

## Loudness, spectrum, envelope and phase observations

Actual29s principal signals before separate constant-gain matching:

| Mix | Raw RMS | Raw peak | Integrated LUFS | Matched gain | Matched true peak |
| --- | --- | --- | --- | --- | --- |
|A serial|.00831555|.0257356|−40.7|7.673615|−14.1dBTP|
|B static|.00848565|.0331604|−40.5|7.498942|−12.1dBTP|
|C dynamic|.00789205|.0307687|−41.2|8.128305|−12.0dBTP|

All matched principal files remeasure **−23.0LUFS**. Meter: FFmpeg7.1.5 ebur128,
BS1770 integrated gating/true peak, mono (no dual-mono compensation),0.1LU output
resolution. Only constant gain is applied by existing PCM reader/writer; **no
loudnorm, automatic compressor/limiter or resampling**. All raw originals retained.
Target would be lowered together if necessary for−1dBTP headroom; here ample
headroom remains.08 has its own disclosed LUFS copy. Wet common-listen06/07 use
one shared7.673615 gain, not separate normalization that conceals ducking.
LUFS matching is a perceptual control, not proof of equal timbre or superiority.

Active20s4096sample Hann/hop2048 spectra, C versus the same C-no-carving:
**20–120Hz power−2.379dB;120–500Hz−1.455dB;500–2500Hz−0.065dB;
2.5–12kHz−0.0006dB**. This supports actual selective carving without broad upper
attenuation. It does not prove bass removal sounds better. Full raw fractions and
constant-gain levels are retained, not only normalized plots.

Actual100Hz-decimated envelope/gain CSV records the sample-wise processing.
Maximum reduction4.000dB; gain stays within[.630957,1]. In2–3s rhythmic activity
it reaches4dB. In actual3.88–4.26s quiet gap it rises .910878→1; in14.22–14.90s
longer gap .982963→1. During15–20s final decay it recovers .769180→1. From20s tail,
C/B return gain is1 and their tails coincide. Window RMS values for all principal
and isolated wet files are in analysis.json. This demonstrates gain recovery and
preserved decay, **not an assertion of subjective decay audibility**.

All parallel branches share the Muff's nominal **48-frame/1ms FIR group delay**.
The optional clean lane uses an actual48sample ring, tested exactly. Circuit phase
and nonlinear histories are not made magically linear by that alignment. Rotor
model propagation intentionally varies roughly2.39–3.44ms(horn)/2.65–3.18ms(drum),
from existing(1±radius)/343 equations; tape delay14400samples/300ms and the hall's
early/late arrivals are musical paths, **not errors to align back onto dry**.
Complete64-frame host blocks add no extra queued block; this is not converter/I/O
latency evidence. No physical latency measured.

For B's actual aligned core and weighted wet streams: cosine≈−.249 and mixed
power≈**−1.232dB** relative to their uncorrelated power sum. Phase cancellation is
present and disclosed; it is not automatically bad or silently polarity-corrected.
PCM16 reconstruction is approximate. A/B compare routing+carving together; the
provided C/08 and C/09 comparisons isolate carving and ducking respectively. They
do not support attributing every A/B difference solely to routing topology.

## Resource evidence and actual bottleneck

Full real graph CPU includes all four engines on every block; no cached processor
output replaces timed work. Final quiet repeat,1392000frames/mode,64-frame calls,
GNU14.2 Release/O3/EPYC9V74. Timing brackets only Graph::process; parsing/preparation,
file IO/stats occur outside. Diagnostic stores remain inside measured processing.
Mean host **A6.342 /B6.105 /C6.154µs/frame**. Scheduler/clock uncontrolled; occasional
observed blocks4–9ms include preemption and are not a WCET/deadline argument.
Two complete renders have byte-identical18 native/matched WAVs; timings differ.

Separate same-stem measured passes: Muff5.692, rotary.0821, tape.0113, hall.2634
µs/frame. New routing-only mixer .01008µs/frame (actual recorded stems, same
filters/envelope/gains). These separate noisy passes are **not an exact additive
cost decomposition**. The Muff is clearly the host CPU bottleneck; optimizing
all effects or blaming the mixer is not justified by these measurements.
Hall is the largest memory component; no algorithm optimization undertaken.

Host persistent processor states: Muff8832B, rotary4392B, tape230528B including
57600float medium, hall474704B. Complete graph+medium **719656B**, including332B
routing and other bookkeeping/block buffers; **1200B extra persistent infrastructure**
beyond those four states. Reused caller diagnostic Block is additional2048B;
call stacks/high-water not measured. Offline full-wave buffers/FFmpeg are host
laboratory memory, not part of that persistent DSP count.

Actual pinned xPack14.2.1 M7 compile of **only new routing infrastructure**:
**1368B text,0data/BSS;332B instance;12B own audio frame**. Audio functions have no
external calls or double math; stopped prepare alone calls tan/exp/pow. ABI-only
sizeof probe records four native states and a719632B graph-shaped aggregate with
medium; it is **not a linked/placed/running ARM graph**. Full graph target flash,
placement, hardware cycles/deadline, stack/heap high-water and audio are **not
proven**. Small fixed float-only mixing is plausibly inexpensive for Daisy;
this does **not** admit the full four-effect graph. Profile the existing Muff on
target first if this routing is musically selected.

An initial local ARM version guard incorrectly looked for packaging suffix
14.2.1-1.1 in --version, which GCC does not print. Corrected this new script to
check actual dumpfullversion14.2.1 and xPack identity, retaining binary hash and
existing hash-pinned recipe metadata. Rejected guard log retained. The same erroneous
guard exists in PR77's audit script: **that separate script needs a narrow follow-up**;
PR77 and its DSP were not changed here. Earlier successful hall link/map remains
valid evidence; this experiment does not rely on its broken guard.

## Validation and reproducibility

Baseline5/5. Final scoped Release **7/7**,2.64s; Debug **2/2**,18.00s;
poisoned ASan+UBSan **2/2**,17.00s. Native checks cover new HPF DC/cutoff behavior,
attack/release response, gain bounds/smoothness, clean48sample alignment, actual
C-no-duck/B equality, block rejection/history preservation and callback guards.
CLI checks exact frozen Hall source, input/parameter rejection and constant-gain
PCM scaling. No new SPICE campaign or exhaustive original-model test rerun.
Existing tests/CI steps retained;452 prior source/firmware/fixture hashes unchanged.

From clean checkout, C++17/CMake>=3.15/Python>=3.9:

```bash
cmake -S parable-starter -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release --target parable_wallace_lab parable_wallace_test parable_soundforge_test parable_core_test parable_audio_safety_test -j2
ctest --test-dir build/Release --output-on-failure -R '^(wallace_|core_validation|audio_safety|soundforge_)'
cmake -S parable-starter -B build/Debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG='-O0 -g1'
cmake --build build/Debug --target parable_wallace_lab parable_wallace_test -j2
ctest --test-dir build/Debug --output-on-failure -R '^wallace_'
cmake -S parable-starter -B build/Sanitized -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-O1 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build build/Sanitized --target parable_wallace_lab parable_wallace_test -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:max_malloc_fill_size=100000000 \
UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/Sanitized --output-on-failure -R '^wallace_'
python3 parable-starter/tools/wallace_input.py --original parable-starter/samples/wallace/source-original-stereo24.wav --output build/wallace-input
./build/Release/parable_wallace_lab --input build/wallace-input/take03-late20s.wav --output build/wallace-auditions
# Offline analysis only: NumPy2.3.5 (Python>=3.11), FFmpeg ebur128; local version7.1.5 recorded.
python3 parable-starter/tools/wallace_evidence.py --directory build/wallace-auditions --lab build/Release/parable_wallace_lab
python3 parable-starter/tools/check_wallace_arm.py --compiler PATH/TO/PINNED/arm-none-eabi-g++ --output build/wallace-arm
```

Locally: build roots`/workspace/work/sf023-{release,debug,sanitized}`, CMake3.31.6
in`/workspace/work/baseline-tools/bin`, existing pinned JSON3.11.3 source override
`-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src`.
Compiler path`/workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++`;
new routing objects actually compiled; no legacy firmware rebuild/full graph link.
Existing firmware/build.py obtains hash-pinned tools on a fresh machine; it is
not necessary to flash anything. Exact ARM argv/hashes, logs and actual analysis
are under[data/sf023](data/sf023/README.md). Different meter versions can alter
0.1LU rounding/matching gains; native WAV hashes characterize observed compiler.

Produced by source commit35a39d5 plus source SHA beside audio; later commits add
recordings/evidence/docs only. First actual serial guitar WAV **11m52.647s** after
assignment download (timing receipt); not a claim of founder-approved usable sound.
Credits/billing are not exposed. Native algorithm changes **zero lines**; exact
Hall selected core and unchanged main3, existing PCM/stats/CHECK/callback guards
reused. Only fixed mixing/filter/envelope/control/lab glue is new.

Changed: exact Hall import+notice/pin; minimal routing/graph/lab/analysis/prep tools;
focused tests; additive CMake/CI and one registry reference13→14; ADR/report/status,
source/audio/provenance. No effect/patch/correspondence/firmware source rewrites.
Remote CI is reported separately in PR checks; local passes are not remote success.

**Next smallest step:** founder compares matched A/B/C, then C against08/09 and
wet isolation. If the console offers little benefit or sounds worse, retain that
outcome. If preferred, measure the existing Muff and this router on actual Seed3
before considering production routing. Fix PR77's version guard separately.
Leave this PR unmerged; no planetary integration decision follows automatically.
