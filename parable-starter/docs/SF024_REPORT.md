# SF-024 — Wallace II, clean guitar through real cabinet measurements

**Primary output: [ten matched listening comparisons and clean solos](../samples/wallace2/README.md).**
The default preserves an independent clean path at gain .50 alongside Muff .75;
clean bypasses all four effects, then passes the shared cabinet. It remains
roughly **6.3dB below the complete mix in RMS**, not a token zero-initialized option.
Clean-disabled06 is explicitly a control. No all-wet principal comparison.
This report establishes routing, cabinet convolution and reproducible audio;
subjective authenticity, articulation improvement and placement preference remain
open listening questions, not inferred from spectra or nonzero output.

## Scope and baseline

[Assignment](SF024_ASSIGNMENT.md), [Gate0](SF024_GATE0.md),
[written experimental decision](adr/ADR-024-wallace-cabinet-clean.md).
Read project rules/Constitution/README/status/Soundforge recipe, existing reports
and actual graph/APIs. No root AGENTS exists. Frozen starting point PR78 head
06e54dbf797c708f46a5890cf97a4c1839952d63, main767a3c4. New branch stacks on
`sf-023` for a focused diff; **77/78 neither modified nor merged**. Before edits,
Release build original Wallace targets and2/2 Wallace tests pass,1.74s.

Exactly unchanged existing Muff, Rotary, Tape and Hall. **462 native/header,
firmware, fixture and selected original utility files hash-protected** against
that starting checkout. Original Muff direct and serial raw WAVs match SF023
byte-for-byte. No Engine, schema, correspondence, firmware, living-source,
physical device or nonlinear solver change. No unrelated SPICE campaign.

## Source and licensing

Full mono measured cabinet IRs by **jesterdyne**, March18,2011, CC BY4.0:
[Jensen Cab SM57 center](https://freesound.org/people/jesterdyne/sounds/116743/),
[Engl Celestion V30 SM57 center-01](https://freesound.org/people/jesterdyne/sounds/116735/).
Original author pages explicitly permit share/adapt with credit. Lossless bytes
match two pinned public repositories; exact commits/paths/hashes and author credit
are in [IR_NOTICE](../samples/wallace2/IR_NOTICE.md). No proprietary plugin code,
commercial IR guess or login bypass. Names are published filenames: original
H&K generic description conflicts with ENGL title. **Open/closedback geometry and
exact speaker models are unverified**; preferred4x12 vs open1x12 contrast is not
established. These are documented measurements, not synthetic cab filters.
Rejected synthetic Felitronics/Nave and ambiguously redistributable Science Amps.

Full originals44100Hz/24bit retained. Explicit FFmpeg resampling to48k float32,
scalar unity peak FFT magnitude normalization, no onset trim/minimum-phase/EQ.
Jensen13156frames, V3015000frames. Original recorded phase/timing preserved.
[Preparation receipt](../samples/wallace2/irs/preparation.json) records actual
filter settings/gains/hashes. FFTConvolver automatically omits sub1e-6 trailing
coefficients: Jensen none; V3089frames, L1 omitted2.793e-5. Reference comparisons
include the **full** IR, not a reference secretly trimmed to fit implementation.

Reuse [HiFi-LoFi/FFTConvolver](https://github.com/HiFi-LoFi/FFTConvolver/tree/f2cdeb04c42141d2caec19ca4f137398b2a76b85)
MIT, six selected files,64sample uniform partitions. Original Git blobs verified;
MIT and [transitive Ooura terms/credit](../third_party/fftconvolver/OOURA_NOTICE.md)
retained. Portable Ooura backend, SSE explicitly disabled, no FFTW/Accelerate.
One local adjustment: guard `Buffer::setZero` when size0, fixing the actually
observed UBSan null-pointer memset diagnostic. Original/local hashes and rationale
are both pinned; no nonempty convolution arithmetic changed. Do not suppress UB.
No registry import allowlist expansion or fabricated legal clearance.
Optional amplifier comparison D **omitted**: no credible native drop-in exists;
a new amplifier/JUCE/neural model pipeline would expand this task.

Same Guitar-TECHS take03/excerpt as SF023,9.28–29.28s, mono48k PCM16;
[licensed authors/change credit](../samples/wallace2/GUITAR_NOTICE.md) retained.
Same calibration .2 input peak (gain .646184184579) and9s tail,29s total.
Muff healthy .65/.5/.65 original4x, rotary command .7; Tape1.2s/.3s/.32/2.4,
record4500/replay6000/erase0; Hall healthy decay1/focusing0, same stereo-to-mono.
No time stretch, fake performance, additional notes or re-recorded source.

## Actual routing and mandatory clean proof

Precab05: clean48sample alignment → gain .50; independent Muff → gain .75;
clean-delayed source independently drives wet-only Rotary/Tape/Hall. Return gains
.18/.12/.12, same existing Tape120Hz/Hall180Hz highpasses, no default ducker.
Sum **clean + Muff + returns**, then one common cabinet1.06 removes only direct
clean: sends still receive the same clean source.07 changes only clean .75 and
Muff .45. Independent CLI gain controls; zero clean rejected for principal recipes.
The explicit graph clean-disabled flag is restricted to the named precab control.

Postcab08: (.50 clean48 + .75 Muff) → cabinet1; this complete dry cabinet bus
passes at **unity to the final output**, plus wet Rotary/Tape/Hall driven from it.
Same return gains/HPFs;09 changes only Tape/Hall ducking, max2.5dB,10/250ms,
pre-Muff envelope thresholds .012/.05. Rotary stays unducked, no dry attenuation.
10 is separately labeled expansive, returns .25/.32/.40, direct path still present.
Postcab Rotary is a studio experiment, not a physical Leslie reconstruction.

Independent weighted clean-beforecab/aftercab, fuzz-aftercab and final-ambience
stems are exported for05/07/08/09/10. Extra diagnostic convolutions run **outside**
the timed graph, not additional live cabinets. Their sum reconstructs the actual
mix with maximum error **4.85e-8** in double analysis; the actual exported PCM16
listening stems reconstruct within **one quantization LSB**, also independently
checked, including the shared-gain clean-disabled subtraction; pre clean-on minus clean-off
matches the isolated clean-aftercab within **3.36e-8**. The post dry cabinet bus
is explicitly added, never replaced by wet returns. Unit tests independently
verify clean=exactly .50×48sample-delayed input and actual graph routing.

Default weighted clean aftercab RMS .008687 over active20s, versus full .018087
pre / .017884 post: **−6.37 / −6.27dB**. This is a meaningful contribution; solo
WAVs retain each fullmix's listening gain, no independent solo normalization.
Clean/fuzz cosine .7377; combined power +2.336dB versus incoherent sum. Correlated
addition/phase remain, not automatically polarity-corrected. They are not a sonic
quality score. Clean is unprocessed by effects, **cabinet-colored** at output;
there is deliberately no unfiltered bypass around the shared cabinet.

Cabinet introduces **0 extra buffering frames**, tested with delta kernel and
independent direct time-domain convolution across irregular chunks/partition ends,
reset and complete tails. Recorded IR peaks9frames(Jensen),156frames(V30) remain;
−60dB-relative first arrivals3/125frames. Wholemix shares one cabinet, so no
additional parallel cabinet latency compensation is needed. Clean48 compensates
Muff's nominal FIR group delay only, not all physical circuit phase. Rotor
propagation,300ms echo and room arrivals are preserved as musical behavior.
No physical converter/roundtrip latency was measured.

## Listening fairness and three questions

All10 fullmixes measured **−23LUFS** (FFmpeg ebur128,0.1LU meter resolution),
constant scalar gain only; all true peaks ≤−7.7dBTP. Matched from native float
intermediates before PCM16 quantization; no loudnorm, limiter/compressor or
waveform-dependent mastering. Raw PCM16 copies and gains are retained. Original
serial and Muff raw PCM are exact SF023 controls.05/06/07 also share one identical
listening gain, so independently matched06 cannot hide its actual4.4LU raw loss.
Duck min gain .749894 corresponds to2.5dB; only wet Tape/Hall change. Integrated
output RMS need not decrease monotonically when correlated returns are attenuated.

1. **Cabinet authenticity/usefulness:**01 versus02/03 supplies the controlled
   cabinet-only evidence. Measured cabinet attenuation at10kHz is about28–29dB,
   with different resonant/midrange structure, not just a generic lowpass. Whether
   this substantially improves perceived authenticity is **not human-verified**.
2. **Articulation/composite:**05/06/07 matched and common-gain versions plus
   clean solo prove the substantial independent path. Improved perceived pick/
   chord clarity remains a listening judgment, not established by RMS alone.
3. **Placement preference:**05/08 and08/09 isolate the specified routing/ducking
   choices with fixed settings. The effects' excitation changes by design; no
   claim one is universally better. Preference **not adjudicated here**.

Audio rendered and numerically checked; no claimed founder, experienced guitarist,
physical-pedal/amp A/B or controlled human musical approval. This does not block
delivering experimental audio or imply a finished rock-recording quality verdict.

## Actual host/ARM resources

GNU14.2 Release/O3, AMD EPYC9V74,48k/64blocks,1392000frames per mode;
actual full processor execution timed, not cached Muff/return playback.
Final deterministic repeat: convolution-only **Jensen .08436 / V30 .09475µs/frame**;
full05 **6.3297**,08 **6.7032**,09 **6.4038µs/frame**. First pass was .08778/.10253
and full05/08/09 6.4916/6.4490/6.1326. Separate passes/scheduler noise mean these
are not an exact additive cost breakdown. No affinity/frequency lock or deadline
claim. Observed block peaks7.48–11.09ms final repeat include host scheduling;
initial expansive block50.6ms retained. These are observations, not WCET bounds.

Measured requested live new/new[] payload, including processor object/spectra/history:
**Jensen245452B; V30276556B**. Full graph incl four processors/tape medium/cabinet:
**966292B /997396B** respectively; caller diagnostic block1280B additional.
Peak during stopped prepare equaled retained payload for this allocation sequence;
all tracked allocations released. Allocator headers/metadata, call stack, file I/O,
full-wave render buffers and input-kernel storage excluded. Kernel files52624B /
60000B are separate caller preparation data; processor does not retain that input.
Gross requested allocation traffic249532/280636B is also retained in unit log;
do not confuse it with live RAM. No callback allocation/free/lock/I/O in guard test.

Pinned xPack ARM14.2.1 object compilation: FFTConvolver3673B text,
AudioFFT5748B, Utilities890B, **10311B sum**,0data/BSS. This sum includes unlinked
functions/metadata, not firmware flash placement. Ooura uses double FFT work
arrays/math (333 static .f64 instruction occurrences across whole objects),
plus control-side allocation/exception/runtime symbols. **Not float-only DSP.**
Disassembly/stack/undefined-symbol/compiler/hash/argv receipts in[data/sf024/arm](data/sf024/arm/report.json).
No wholegraph link, SDK placement, flash/SDRAM admission, M7 cycles, callback
stack high-water, boot/analog audio or physical Seed3 run. Host success does not
make this Daisy-ready. No production firmware change or existing ARM-image rebuild.

## Tests, exact reproduction, limitations

Before edits original Wallace **2/2**. Final scoped Release **9/9**,5.97s;
Debug original+new Wallace **4/4**,34.63s; ASan+UBSan **4/4**,33.55s.
Includes584 cabinet/actual routing assertions, two measured full kernels versus
independent double time-domain reference, chunk boundaries/tails/reset, exact delta
latency, malformed controls/kernels/input, no-allocation/lock/I/O callbacks,
all462 protected files, existing core/audio/shared utility/registry checks.
Full-recording independent NumPy FFT convolution maximum errors **1.71e-8 /9.41e-7**,
relative RMS1.75e-7/3.12e-5; V30 difference includes declared tiny tail omission.
Two complete renders then final patched-library repeat reproduce all30 raw WAVs
byte-for-byte, including contribution stems. Logs retained. Initial UBSan failure
fixed; initial sparse-checkout registry failure was missing old evidence, resolved
by restoring its unchanged file; no registry logic was weakened.

From clean checkout of `sf-024`, C++17/CMake>=3.15, Python>=3.11 for NumPy2.3.5;
prepared IRs and the existing input are committed, so no IR/login/download needed:

```bash
cmake -S parable-starter -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release --target parable_wallace2_lab parable_wallace2_test parable_wallace2_memory parable_wallace_lab parable_wallace_test parable_core_test parable_audio_safety_test parable_soundforge_test -j2
ctest --test-dir build/Release --output-on-failure -R '^(wallace_|wallace2_|core_validation|audio_safety|soundforge_)'
cmake -S parable-starter -B build/Debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG='-O0 -g1'
cmake --build build/Debug --target parable_wallace2_lab parable_wallace2_test parable_wallace_lab parable_wallace_test -j2
ctest --test-dir build/Debug --output-on-failure -R '^(wallace_|wallace2_)'
cmake -S parable-starter -B build/Sanitized -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-O1 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build build/Sanitized --target parable_wallace2_lab parable_wallace2_test parable_wallace_lab parable_wallace_test -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:max_malloc_fill_size=100000000 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/Sanitized --output-on-failure -R '^(wallace_|wallace2_)'
./build/Release/parable_wallace2_lab --input parable-starter/samples/wallace/take03-late20s.wav --irs parable-starter/samples/wallace2/irs --output build/wallace2-auditions
./build/Release/parable_wallace2_lab --input parable-starter/samples/wallace/take03-late20s.wav --irs parable-starter/samples/wallace2/irs --output build/repeat-receipts --verify-against build/wallace2-auditions
./build/Release/parable_wallace2_memory parable-starter/samples/wallace2/irs
# Analysis: external FFmpeg ebur128 and NumPy2.3.5, not linked DSP.
cp -r parable-starter/samples/wallace2/irs build/wallace2-auditions/
python3 parable-starter/tools/wallace2_evidence.py --directory build/wallace2-auditions --discard-floats
# Optional rebuild prepared kernels, originals remain unchanged:
python3 parable-starter/tools/wallace2_ir.py --directory parable-starter/samples/wallace2/irs
python3 parable-starter/tools/check_wallace2_arm.py --compiler PATH/arm-none-eabi-g++ --output build/wallace2-arm
```

Actual local roots `/workspace/work/sf024-{release,debug,sanitized}`;
PATH `/workspace/work/baseline-tools/bin`(CMake/CTest3.31.6), existing pinned JSON
source override `-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src`.
Python3.12.14, NumPy2.3.5, FFmpeg7.1.5-0+deb13u1. ARM compiler exact path
`/workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++`.
Exact commands/hashes/logs beside report. Different meter/compiler versions may
change last bits/0.1LU values; deterministic equality is for this observed build.
New additive focused CI job retains every prior job; remote result reported in PR,
not inferred from local success. Artifact files are committed for direct download
independent of CI publication.

Changed files/reasons: reusable independent Cabinet + pinned convolver/notices;
fixed new pre/post host graph/lab using original router/effects; IR prep/meter/
ARM/payload tools; focused reference/routing/CLI tests and additive CMake/CI;
full licensed IRs/30 raw,10 matched,3 common-gain,20 same-gain solo WAVs/receipts;
Gate0/ADR/report/status/README. No new general mixer, amp or effect algorithm.
Borrowed convolution/FFT, existing PCM/statistics, exact four engines and previous
return mixer;273 new own C++ lines incl test/memory runner,0 original effect lines
changed. Git diff and pin manifests provide the actual reuse record.

Unfulfilled/unproven: preferred openback/closedback geometry contrast, optional
amp D, human sonic verdicts, varied performances/guitars, stereo studio image,
physical captures/equipment fidelity, full-target resource/runtime admission.
Control-side convolver initialization allocates and can throw; lifecycle remains
stopped/single-owner host, no live replacement/activation guarantee. Those are
future admission questions, not newly promised production infrastructure.

**Next smallest step:** play05/06/07 and05/08, with clean solo/common-gain controls
available; select a useful experimental recipe. If selected for hardware research,
measure existing Muff plus one cabinet on actual Seed3 before any fullgraph
admission or production routing commitment. No further effect model work required.
Leave this experimental PR unmerged.
