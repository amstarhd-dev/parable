# SF-022 — Leo grand hall / one coherent reflecting plane

Experimental standalone voice. [Assignment](SF022_ASSIGNMENT.md),
[pre-edit audit](SF022_GATE0.md), [decision](adr/ADR-022-leo-grand-hall.md),
[listening package](../samples/hall/README.md). Base/current main at start:
`767a3c402b972fc20df8aea0729adf794f4b7a38`. PR #76/Cancer is unchanged/unmerged.
No Engine, patch/schema, storage, living modulation, correspondence or production
firmware change. **Leave this PR unmerged for independent review.**

## Result to listen to

The healthy processor is the **complete Airwindows kCathedral**, not a new tank
inspired by it. Its original early reflections, five-stage five-path late network
per channel, bass treatment, filters, cross-channel feedback, output gain law,
rate interpolation and floating dither remain. It receives mono input and has
actual stereo output; the normal mono output is the stereo arithmetic mean.
The direct stage stays at0.65 dry; architecture/late field share0.35 wet.
No shimmer, pitch shifter, additional chorus or invented acoustic dimensions.

Original and adopted healthy native float samples agree **exactly** for nonzero
2s signals through early/late networks at44.1/48/96k. The real-guitar wet PCM files
are also byte-identical. Original plugin random seeds are fixed to the observed
Linux/glibc srand7 pair in the native core; no callback rand/global generator.
Other platforms/math libraries are not promised byte-identical recordings.

Both complete original candidates ran the same take03 first8s+8s tail at48k,
with0.65dry+0.35mono-wet and separate whole-file RMS copies:

| Original candidate | T20-extrapolated mono decay | Host sizeof / observed allocator delta | Full-process median µs/frame |
| --- | --- | --- | --- |
| kCathedral, Wetness1 | 4.894s | 474752 /475008 B | 0.305561 |
| Verbity, Size.8/Longness.85/Darkness.3/Wetness1 | 4.949s | 1007232 /1007488 B | 0.0685914 |

The first Verbity trial was shorter2.595s; adjusting its own existing controls
produced the fairer4.949s comparison. It remains much cheaper. On identical mono
excitation its symmetric dual-mono network produces identical L/R, whereas
kCathedral preserves spatial difference. The selected IR has12.35% energy before
80ms versus0.70% for Verbity, with substantial late energy after that. Its first
sample above−60dB of IR peak is27.29ms versus75.42ms. These characterize distinct
architecture; they do **not** establish an objective musical-quality ranking.
Selection favors explicit early structure, stereo development and long hall decay.

[MIT source pins, exact per-file hashes and transitive audit](../third_party/hall/provenance.json).
Airwindows pin`143a86df3642881e0be4bf827815050721063b41`, Chris Johnson.
Native port removes only VST metadata/host dependencies and duplicate double-I/O
entry point, retaining the original float-I/O processor's double arithmetic.
The research reference compiles the untouched original files against a small local
host/type shim; no Steinberg SDK is imported. `hall_adopt.py --check` reproduces
and verifies the adoption. Upstream warnings are retained, not silently patched.

Freeverb3 Progenitor/ZRev2 and MVerb were surveyed but not imported under the
current MIT/BSD2-only policy (their sources are GPL). Signalsmith Stretch does
pitch/time; Basics has a real reverb but complete STFX wrapper license/atomic64
integration work was not adopted. Two C Dattorro plates were inspected/rejected:
fixed-delay-rate el-visio and a missing left-tank delay write in johnhw. Exact
revisions/reasons are recorded. No license policy changes or GPL source import.

## Same-system fault and its limits

Only the final existing scattering plane changes: normalized Householder H becomes
`S=(1−f)H+fI`, f0 healthy /f0.98 damaged. All five outputs of each channel's vector
change together, preserving the original cross-channel connection map and all
delays, loss, input gain, filters and output mix. This reduces diffuse
redistribution and favors coherent existing reflection paths. It adds no
oscillator, separate resonator, level detector, RMS gate or sound layer.

H and I are orthogonal; the convex blend is contractive. Independent NumPy SVD
checks the boundary, not the entire nonlinear engine. The original normalized
late-loop retention factor before its filters is3125×.000293=0.915625 maximum;
the new decay control only reduces it. Full nonlinear global/passive stability is
not inferred from that component argument: native sustained/overload/decay tests
are separate. This is a phenomenological boundary model, not measured concave-wall
geometry or an acoustic material calibration.

The raw guitar fault is not just louder: RMS falls0.030579→0.029207 while its
spectrum/phase/tail change. Same wet-only synthetic110/138.59/164.81Hz sustained
chord has RMS0.006290→0.017259, a conspicuous numerical increase around excited
paths. In independent1–5s IR50Hz bands, relative late energy rises about8dB near
2.8–3kHz. Composite fault T20 is4.308s, so this is selective redistribution, not a
blanket longer-tail switch. Actual audibility/"grand and dangerous" value remains
for listening. Healthy/fault normal recordings have **zero containment hits**.

**Weak playing limitation:** this physically motivated focusing is substantially
linear. Weak guitar difference RMS is0.000740 versus healthy0.000609; it does not
vanish relative to weak signal level. It is quieter in absolute terms, but the
requested "largely normal" weak behavior is **not established**. No artificial
activation threshold was added to manufacture that outcome. Do not claim this
criterion passed. A different amplitude-dependent physical mechanism would need
its own justified decision, not an unreported extra fault.

Reset is **stopped-only** and clears audible history deterministically while
retaining settings. Its listening file explicitly applies an external5ms output
fade around a stopped reset at4s. This demonstrates a clean laboratory protocol,
**not an implemented native live clickless reset**. No switching architecture added.

## Resources: an actual memory/link fit, not a device-performance claim

Fixed caller-owned Hall **474704 B**, including Core474680 B; same host/M7 layout
at all3rates. No vector, delay allocation, hidden heap or second reprepare instance.
Stopped prepare clears the same storage. Callback allocation/I/O/lock guards pass.
On target, placement-new occurs in already initialized SDRAM after Seed::Init.
The static unconstructed buffer avoids touching external memory during startup.

Pinned xPack14.2.1-1.1 + libDaisy`1289a000bc29dae1bc0ed4129cfa2df40ef71574`,
with SDK/submodule identities and linker/archive hashes verified. Actual native
objects: Core8808B text +adapter720B =**9528B**, data/BSS0 in those objects.
Callback includes original double `sin`, `pow`, `frexpf`;8/4/2 static call sites.
No allocator/IO/lock/atomic symbols. Calls are not eliminated merely to improve
an audit; these expensive operations are preserved and target cost is unresolved.

A **complete standalone audio firmware** probe links with the official unchanged
**BOOT_NONE128KiB internal-flash script**, no bootloader migration or enlarged
FLASH. It configures64-frame mono-in/stereo-out and actually calls the native
faulted Hall. [Exact sections, symbol/map, disassembly and commands](data/sf022/arm/):

| Region/section | Bytes | Selected location |
| --- | --- | --- |
| Flash load image, allocated sections including data initializer | **95348** | 0x08000000 internal flash |
| .text | 93644 | internal flash |
| .isr_vector /exidx /init_array /fini_array | 664 /8 /40 /4 | internal flash |
| .data /.bss | 988 /12340 | 0x24000000 AXI SRAM; alignment separately in map |
| .sram1_bss | 16384 | 0x30000000 DMA/D2 SRAM |
| .sdram_bss | **474704** | **0xc0000000 SDRAM**, actual Hall buffer |
| .backup_sram | 12 | 0x38800000 |

The debug ELF file/section total is not flash consumption. GNU summary reports
text93652/data1696/BSS503440; section classes account vector tables differently.
No heap high-water measurement: static section sizes do not establish SDK dynamic
allocation. Known callback frame chain16+304+376=**696B before libm, caller and IRQ**;
not a stack high-water/WCET bound. Engine/bundle/Runtime remain11040/11288/45224B.

Final full native processing host medians **0.315990 healthy /0.298901 fault
µs/frame**,3 sequential768000-frame passes,64-frame calls, GNU14.2/O3/EPYC9V74.
No concurrent DSP builds/tests during these passes; scheduler/clock uncontrolled.
Fault being faster in this small noisy sample is not an optimization claim.
All28 measured original/native WAVs were byte-identical across repeats; every
committed WAV matches the final repeat. Setup/stats/I/O are outside timing.
No translation from host µs to MCU utilization is made.

**Not tested:** physical Seed3 boot/audio, deadlines/cycles including SDRAM/cache
and double libm, codec latency/noise, stack/heap high-water or any analog acoustic
fit. This evidence passes the fixed-memory/link feasibility screen. It does not
admit the effect to production Daisy playback, especially at96k.

## Focused validation and reproduction

Baseline before edits: all existing **40/40 Release**,142.85s. Final complete
**43/43 Release**,371.19s with concurrent test/build activity. Focused Debug
**6/6**,57.02s; poisoned **ASan+UBSan3/3**,90.86s with leak detection.
144 native checks include exact upstream3rates, audible-history reset, stereo/mono
energy, invalid block/control history preservation, sustained20s decay at both
control endpoints and f0/.98/1, finite overload, source-noise floor and callback
guards. CLI verifies18 native WAVs, PCM/stereo shape, repeatability and rejection.
Source check verifies original pins and generated adoption. Registry remains offline.

A full unchanged Debug campaign was started, then intentionally stopped after
31/43 completed without failures: the complete Release regression already passed.
The final Debug campaign covers all new hall tests and shared Soundforge utilities.
**No complete43/43 Debug claim.** Partial log is retained; prior tests/CI are not
removed or skipped in repository configuration. Sanitizer reference compilation
was slow; it completed with original O1 flags, without weakening checks.

Decay endpoints using an actual two-sample native transient (T20 extrapolation):
44.1k2.84–5.34s;48k2.61–4.91s;96k2.60–4.90s. Control is normalized loop retention,
not a promised calibrated RT60 setting. The48k one-sample source comparison uses
4.894s. Initial96k phase0-only impulse missed the upstream every-other-sample input
read and measured noise-floor accumulation instead of decay; that invalid result
is retained/labeled **rejected**. The corrected two-sample probe excites both
phases. This phase sensitivity and incomplete input antialiasing are inherited,
not quietly fixed; quantified nonlinear/decimation alias rejection is unproven.

From a clean checkout with C++17, CMake>=3.15, Python>=3.9 (ARM recipe>=3.12):

```bash
cmake -S parable-starter -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release -j2
ctest --test-dir build/Release --output-on-failure -j2
cmake -S parable-starter -B build/Debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG='-g1 -O0'
cmake --build build/Debug -j2
ctest --test-dir build/Debug --output-on-failure -R '^(hall_|soundforge_)'
cmake -S parable-starter -B build/Sanitized -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-O1 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build build/Sanitized --target parable_hall_test parable_hall_lab -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:max_malloc_fill_size=100000000 \
UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/Sanitized -R 'hall_' --output-on-failure
./build/Release/parable_hall_lab --input parable-starter/samples/hall/input_take03_20s.wav --output build/hall-auditions
./build/Release/hall_candidate_kCathedral parable-starter/samples/hall/input_take03_20s.wav build/hall-original
./build/Release/hall_candidate_Verbity parable-starter/samples/hall/input_take03_20s.wav build/hall-verbity
cmp build/hall-auditions/healthy_wet.wav build/hall-original/wet.wav
python3 parable-starter/tools/hall_benchmark.py --build build/Release --input parable-starter/samples/hall/input_take03_20s.wav --output build/hall-benchmark
python3 parable-starter/firmware/build.py
python3 parable-starter/tools/check_hall_arm.py \
  --compiler parable-starter/build/arm-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ \
  --libdaisy parable-starter/build/arm-cache/libDaisy --output build/hall-arm
```

Locally the same commands used `/workspace/work/sf022-{release,debug,sanitized}`,
pinned JSON source override`/workspace/work/sf015-deps/nlohmann_json-src`,
CMake3.31.6 in`/workspace/work/baseline-tools/bin`, and already built verified ARM
cache`/workspace/work/par005-fresh-cache`. The old firmware recipe was **not rerun**
locally; the new script actually compiled/linked the probe using that pinned cache.
All actual ARM argv and source/linker/archive identities are in its report.

Optional offline metrics use **NumPy2.3.5** (not DSP/build dependency):

```bash
python3 parable-starter/tools/hall_metrics.py --selected build/hall-auditions --verbity build/hall-verbity --output build/metrics.json
c++ -O2 -std=c++17 -Iparable-starter/include parable-starter/tools/hall_response.cpp build/Release/libparable_hall.a -o build/hall-response
./build/hall-response build/hall-response-data
```

[data/sf022/README.md](data/sf022/README.md) records the complete rate-analysis
command. Audio is CC-BY4.0 Guitar-TECHS take03, exact prepared-input hash/changes
and authors in the package. Source commit`fce7ec4` plus per-file SHA identifies
producing DSP/render code; later commits publish artifacts/docs/response probe only.
Remote CI results belong in the PR checks/description; local passes are not CI passes.

## Changes, reuse and remaining acceptance

- Complete pinned MIT source/notices +mechanical native extraction;32-line owned
  adapter, fixed-state public interface. Existing DSP/patch contracts untouched.
- Reuse unchanged PCM16/analysis/activation guards;61-line selected renderer and
  original-engine comparator, independent metrics/fixtures and reproducible ARM link.
- Focused native/CLI/source checks, additive CMake/CI/audition artifacts and one
  registry entry13→14 (inventory assertion only). **452 prior implementation,
  firmware and fixture SHA entries unchanged**, all old CI steps retained in order.
- ADR, source/license and audio provenance, status, report, logs/resources/WAVs.
  Existing main has no generic ambience/reverb implementation to compare: no
  historical production baseline or quality superiority is fabricated.

Measured reuse:2465 lines of original code retained verbatim (six files);976 lines
of native state/core are mechanically extracted, including the whole algorithm.
Only the loop control, final10 scattering expressions, control/seed/reset plumbing
and host removal differ. No new allpass/FDN engine, WAV framework or solver.
LOC describes reused code, not a fabricated percentage of engineering time saved.

| Acceptance | Evidence / unresolved part |
| --- | --- |
| Traceable compatible source | Passed source/hash/notice audit under current MIT policy; no global Parable license inferred. |
| Meaningful quality advantage | Complete fairer borrowed-engine recordings exist; professional/previous-ambience superiority **not established**, prior main baseline absent. |
| Audible causal focusing | Same-system mechanism/numerical resonance change implemented; musical audibility and weak-state normality **pending/not established**. |
| Stability / boundedness | Focused3rate sustained/overload/decay and guards pass, no normal containment hits; not exhaustive or a full nonlinear stability proof. |
| Daisy deployability | Actual fixed-memory placement and full original128KiB audio link pass; **physical timing/boot/audio remain untested**. |
| Reproducible audio / validation | Committed original/adopted/fault raw/RMS/stereo/wet/probes +pins/commands; local results above. Remote CI separately reported. |

Next smallest test: independent healthy/fault stereo+mono listening, then a standalone
Seed3 cycle-counter/audio run at48k with this exact core before optimization or
Engine admission. If weak-normal behavior is mandatory, resolve that physical
mechanism decision explicitly. Do not begin a larger architecture or another fault.
