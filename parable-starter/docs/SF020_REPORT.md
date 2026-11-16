# SF-020 — experimental Gemini coupled rooms / one return polarity fault

Issue73. Base main767a3c402b972fc20df8aea0729adf794f4b7a38. One standalone native
mono specimen; no Engine/patch/runtime/firmware/storage/correspondence integration.
PR48 and PR72 remain untouched/unmerged. Host evidence is not hardware proof.

## Playable result and limits

[Listening package](../samples/coupled_rooms/README.md):19 actual native WAVs plus
approved take03 source, original credit/license receipt, settings, raw levels,
explicit matching gains, source identities and hashes. Start healthy_matched and
fault_matched, then wet_only and isolated A/B. Same8s guitar plus4s tail; impulse
responses4s. Native PCM writer refuses nonfinite/clipped outputs. No arbitrary
noise/crash layer or second effect. Founder perception of two rooms and musical
value of the fault **remain unverified**; these numerical gates cannot approve it.

First actual healthy guitar file mtime2026-11-16T23:22:47.219112Z, prototype
commitd8db0ef. Exact task-start time was not instrumented, so no exact total-time
claim. Earliest clock receipt23:18:20Z. Initial aperture.35 was too subtle by
numerical inspection:~7.85% raw guitar difference. Increasing the SAME connection
to.85 yielded36.78% raw difference,48.21% wet-only. Best constant-gain fit leaves
36.74% and48.12% residual respectively: this is not just a level shift.
The pair still differs by only one return sign. Raw RMS healthy.00953607,
fault.00869537; peaks.0626841/.0660855. Separate matched gains5.24325/5.75019
produce0.05 RMS, peaks.328669/.380004. The fault changes raw energy; it is not
silently compensated. These comparisons use delivered PCM16, including tails.
Final producing code a1f6478 is recorded in samples/SOURCE_COMMIT and source hashes.

## Model / one-wire mechanism

[Gate0](SF020_GATE0.md) committed before DSP; [ADR020](adr/ADR-020-gemini-coupled-rooms.md).
Each room has four integer propagation delays and its own path losses and
absorption. A:11.3/13.7/17.9/19.7ms, nominal uncoupled LF RT60.65s, pole5.5kHz.
B:29.3/37.1/43.7/53.1ms,1.9s,3.2kHz. Delay rounding, absorption, readout and
coupling alter measured composite decay; those labels are not fitted building
measurements. First traversals provide distinct early fingerprints. No image-source
or actual dimensions are asserted. Only input A is used in primary listening.

Read eight delayed waves, one-pole absorb each, reflect each group by
H=I-(1/2)11^T. Connect just the two channel0 returns:

```
room A -> local reflected port A --(1-c)--> A write summer -> A propagation
room B -> local reflected port B ----c---> A write summer
                                    ^ ONLY this electrical pickup lead reverses
room A -> local reflected port A ----c---> B write summer -> B propagation
room B -> local reflected port B --(1-c)--> B write summer
```

Healthy connection [[1-c,c],[c,1-c]], faulty [[1-c,-c],[c,1-c]]. No changes to
decay, gains, geometry, taps, mix or family. The identified electronic return is a
reduced signal-path interpretation; this is not a claim that an acoustic doorway
has a reversible wire. All feedback traverses positive-length room delays.
Room readout=.5*(v0-v1+v2+v3), mono wet=.5*(A+B), fixed throughout.

Each H is orthogonal. The connection is(1-c)I+cS, where ||S||2<=1 even through
polarity smoothing,0<=c<=1. Hence ||C||2<=1. Strict per-path losses<1 and stable
nonexpansive absorption give a stable feedback operator, including both fault
states and intermediate controls. The aperture is an intentionally dissipative
reduction, not exact lossless architectural scattering. Four lines/room and
integer delays limit diffusion and geometric realism; no stereo localization,
frequency-dependent boundary impedance, environmental noise or physical fit.
Mathematical FDN/scattering background: [Smith1985](https://quod.lib.umich.edu/cgi/p/pod/dod-idx/new-approach-to-digital-reverberation-using-waveguide.pdf?c=icmc;idno=bbp2372.1985.009;format=pdf).

## Borrowing and actual reuse

Inspected Signalsmith Basics reverb.h and MIT LICENSE.txt at
369e906e03760ec219d07b06fa06dc4fd099f377; exact SHA pins and retained notice are
in third_party/coupled_rooms. Adapted mixed-feedback Householder pattern, not
STFX/framework, modulation or its delay dependencies. The older example2798498
had no identified root license and is audit-only. Full framework import adds
unneeded dependencies and does not provide explicit room-to-room causality.
No other unmerged effect implementation was imported.

New DSP implementation78lines +39line public header; renderer72lines. Directly
reused unchanged lab_wav.h, soundforge/analysis.h, rat_measure.h::fft/fields,
activation_guard.cpp/.h and test_check.h; ARM audit conventions adapted from merged
check_scorpio_arm.py. No new general DSP/PCM/stats/FFT/solver library. Research,
first-WAV, narrow tests and artifact reuse avoided exhaustive legacy reruns.
Initial aperture experiment is retained as a failed distinctiveness candidate,
not counted as musical acceptance.

## Observed validation

[data/sf020](data/sf020/README.md) preserves exact logs/receipts.

- Pre-edit shared utility/registry3/3 pass. Final Release4/4(.66s), Debug4/4(3.30s),
  affected ASan+UBSan2/2(3.46s), leak detection on. No local test failures.
- Independent standard-library Python double dense D*C*H recurrence; no native
  implementation imported.5138 fixture rows,15414 compared values across both
  signs and all three rates; max error2.04e-9 (gate2e-6). This validates the
  declared equations, not independently measured architectural acoustics.
- A-only input gives exact B silence with aperture0. At48k first A response542
  samples(11.292ms); coupled B1948(40.583ms). Reverse excitation transfers too.
  Independent closed B starts1406(29.292ms). Both rooms keep their own delays.
- Fault first differs in A2490samples(51.875ms), then B3896(81.167ms), proving
  delayed recirculation through both rooms. Aperture0 healthy/fault are bit equal.
- Gain-fitted impulse residual39.28%.262144-point FFT:80Hz–10kHz, healthy magnitude
  >.01,14.94% of eligible bins differ>3dB;5/50/95percentile change−4.16/.016/+4.02dB.
- Eighteen rate/aperture0/.85/1/fault corners:5.5–6s impulse energy<1e-5 of.2–.7s.
  This tests unforced decay; no infinite-tail or arbitrary physical room claim.
- Finite/extreme input containment, invalid prepare/control preservation, exact
  reset repeatability/silence, untouched inactive model, and guarded processing
  without heap/IO/locks. Input safety clamp±1 is deliberate; normal guitar peak.15
  stays linear. Values below1e-25 are flushed to avoid denormal tails.
- Stopped rate/geometry prepare/reset; live single-owner mix/aperture/polarity
  use20ms exponential ramps. One48k control-change probe first-frame excess step
  7.29e-6 normalized. No cross-thread UI interface or universal clickless admission.
- Fixed-control8.004kHz linear coherent probe unwanted/fundamental−136.08/−135.82dB
  healthy/fault. No nonlinear alias source below input containment; these are
  numerical spurs, not alias-free certification of overload or live modulation.
- Native CLI19 rendered PCM files, exact coupling-off silence, bitwise rerender,
  malformed/unknown arguments rejected. Source and all deliverable hashes retained.
-452 existing source/header/firmware/fixture files unchanged; all prior CI steps
  preserved in order. No unchanged long-running circuit suites run locally.
  Existing global CI remains enabled, with additive specimen/sanitizer/ARM/artifact
  steps. Remote status belongs to PR checks; local passes do not prove remote CI.

## Host and M7 resources

GNU14.2 Release on AMD EPYC9V74, three full576000-frame guitar passes per mode,
no simultaneous builds: median healthy.02259us/frame, fault.02165us/frame. Includes
actual complete process and three diagnostic stores, excludes file IO. Ranges
.02210–.02321/.02156–.02722us/frame; clocks/scheduling uncontrolled. No eliminated-
call proxy, hardware cycle estimate or WCET claim.

| rate | active float delay bytes | M7 object+active buffer |
| --- | ---: | ---: |
|44.1k|39828|40032|
|48k|43356|43560|
|96k|86708|86912|

Pinned xPack ARM14.2.1-1.1 actual M7 object text1612B,data0,BSS0. Processor204B,
caller-owned maximum88000B buffer; host processor216B. Buffer is partitioned to
actual rate, never allocated during callback. No copying a live instance. Audio
work:8 delay reads/writes and absorption states,2 four-port mixes, one connection,
3 control ramps; no iterative solver/search. Compiled process/setControls have
**no calls or double operations**. expf/powf/memset only stopped prepare/reset.
Actual process static frame128B, no callees; excludes caller/IRQ/context margin.
Engine/bundle/Runtime remain11040/11288/45224B. No firmware link or memory-placement,
M7 cycles, SDRAM/cache/stacks, physical boot/IO/latency/noise measured.

## Reproduce from clean checkout

CMake>=3.15,C++17,Python>=3.9. Run at repository root; configure fetches existing
pinned nlohmann_json3.11.3. In the observed workspace only, offline cache override
was `-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src`
and CMake3.31.6 was on PATH. That cache contains the public dependency only.

```bash
cmake -S parable-starter -B build/sf020-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/sf020-release --target parable_coupled_rooms_test parable_coupled_rooms_lab -j2
ctest --test-dir build/sf020-release -R '^(coupled_rooms_.*|soundforge_registry|soundforge_registry_negative)$' --output-on-failure --no-tests=error
cmake -S parable-starter -B build/sf020-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/sf020-debug --target parable_coupled_rooms_test parable_coupled_rooms_lab -j2
ctest --test-dir build/sf020-debug -R '^(coupled_rooms_.*|soundforge_registry|soundforge_registry_negative)$' --output-on-failure --no-tests=error
cmake -S parable-starter -B build/sf020-sanitized -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -O1' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build build/sf020-sanitized --target parable_coupled_rooms_test parable_coupled_rooms_lab -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/sf020-sanitized -R '^coupled_rooms_' --output-on-failure --no-tests=error
python3 parable-starter/tools/coupled_rooms_reference.py --output /tmp/sf020-reference
diff -r parable-starter/tests/fixtures/coupled_rooms /tmp/sf020-reference
./build/sf020-release/parable_coupled_rooms_lab --input parable-starter/samples/coupled_rooms/input_take03_20s.wav --output build/sf020-auditions
# Repeat this render three times; levels.csv reports actual full-process cost.
# Obtain the unchanged pinned toolchain with firmware/build.py or its manifest.
python3 parable-starter/tools/check_coupled_rooms_arm.py --compiler /path/to/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --output build/sf020-arm
```

## Changed files / next smallest test

include/src: standalone bounded model. tools/lab/reference/ARM: native render and
independent/audit evidence. tests/fixtures: focused numerical/safety/PCM checks.
third_party: exact provenance/license/audio credit. samples: source and downloadable
WAVs/receipts. CMake/CI/registry(+inventory13→14): expose new isolated specimen;
README/status/ADR/Gate0/data: trace decisions, evidence and limitations.
Open-branch CMake/registry appends need reconciliation only after those branches
merge; none imported now. Constitution/astrology unchanged.

Next smallest test: play this candidate on Seed3 in a separately approved
standalone admission/bring-up task, with measured callback cycles and simple
aperture/wet/bypass control. Record whether the two-room character and single
polarity fault are useful while playing. If distinction is insufficient, report
that outcome and revise/reject this precise connection hypothesis. It is not
musically accepted or production-ready now.
