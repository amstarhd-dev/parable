# SF-021 — Cancer chamber and one microphone-return gain fault

Experimental standalone host specimen, issue75. Base main767a3c4. No Engine,
canonical patch, runtime, storage, firmware or correspondence changes. PR48/72/74
remain untouched and unmerged. The issue remains open: physical admission,
independent remote checks and several listening/reference criteria are pending.

## Listen first

[Listening index](../samples/chamber/README.md). Start with selected/healthy_matched
and selected/fault_matched. Also retain the raw pair: the fault changes actual
energy, and separate RMS matching is explicitly labeled. All recordings use the
actual selected native C++ implementation, not a Python audio approximation.

Before fault implementation, complete pinned Cloud Seed and Signalsmith engines
rendered the same take03. Gate0 was committed first; Gate1 recordings/decision
were committed and published before fault work. Founder listened to the initial
pair and chose Cloud as “less subtle, very ethereal.” That is source preference,
not approval of every later room setting or the damage. The original ethereal
comparison remains available and can be prepared with decayControl.43484034.
The selected shorter chamber defaults to.25; all healthy/fault pairs use exactly
the same room. Final shorter-room/damage listening approval is still pending.

## Borrowed sources and adaptation

- **Selected:** complete CloudSeedCore deb21ded9eb7dad9b3ff94ce1ba96a963716594e,
  MIT, original notices retained. Twelve late lines/channel, original taps,
  diffusion, interpolation, damping and modulation; stereo is folded mono.
  This is the complete algorithm, not its scattering mathematics rewritten.
- **Comparison:** complete Signalsmith Basics369e906, DSP v1.7.1 at2d201619,
  original default/ambient and parameter retunings. Exact full revisions and all
  downloaded-file SHA/size pins are in third_party/chamber/sources.json.
  Original candidate defaults/factory presets are preserved, not relabeled chambers.
- Basics root MIT does not erase bundled STFX's separately declared Boost license.
  Umbrella linear0.6.4 is MIT; hilbert-iir1.0.0 is0BSD. They are not direct reverb
  processing dependencies. Full wrapper M7 process calls non-lock-free64bit atomic
  loads. No STFX/DSP/linear/hilbert code is vendored into the selected runtime.
  See [separate policy proposal](SF021_LICENSE_POLICY_PROPOSAL.md); no allowlist
  change, fake MIT label or copyleft import was made. BSD-3 is not called copyleft.
- FFTConvolver f2cdeb04c42141d2caec19ca4f137398b2a76b85/COPYING was inspected.
  A convolver license supplies no rights to IR data. No convolution comparison
  was run because an exact, cleared original chamber IR was not obtained.

Cloud's sample constructs a global static core; static zero-initialization hides
control members that its constructor does not initialize. Dynamic construction
under poisoned ASan exposed an invalid bool, then seed multiplication overflow.
The adopted copy explicitly initializes affected control/seed/Biquad fields.
Exact initialization.patch and per-file original/local SHA records are retained.
Two nonportable std::sinf calls become std::sin(float), preserving single-precision
argument conversion; compatibility.patch records this. No topology, propagation
buffer capacity, equation or DSP processing order was changed. With return
removed, the actual take03 microphone PCM is **bit-identical** to the pinned
original core on this observed compiler/platform. The adopted core passes large
poisoned-allocation ASan/UBSan. No claim that every upstream/platform issue is solved.

Reused unchanged lab_wav.h, soundforge/analysis.h, activation_guard.cpp/.h,
test_check.h and the existing source-preparation recipe. ARM-audit conventions
follow merged main. No new reverb solver, convolution engine, shared DSP framework
or general processor architecture was written. Added DSP is the specifically
requested common return electronics/feedback connection around the whole core.

## Room and fair comparison

The factory DarkPlate is not asserted to be an acoustic chamber. The initial
Cloud retuning had a much longer measured decay than its nominal control label
suggested. Retuning remains within the original algorithm: LateLineSize.2,
LateLineDecay.25, nine early taps over~89ms, early return.5, input low cut120Hz,
high cut6500Hz; other factory diffusion/EQ/modulation settings remain explicit in
src/chamber_cloud_settings.h. These are algorithmic parameters, not physical room
lengths, a measured loudspeaker/microphone or architectural calibration.

Prepared controls: predelay0–80ms, original normalized decayControl0–.5, damping
2000–12000Hz, wet0–1. Preparation/reset/destruction are stopped and single-owner.
Only external return gain may change live, through a20ms ramp on that same owner.
The decayControl is not a promise of measured whole-room RT60. No phone interface
or parallel/concurrent parameter protocol is added.

Original candidate controls, raw levels and presets remain alongside a later
fresh-impulse envelope comparison. Compact comparison mixes use35%dry plus65%
explicitly RMS-calibrated wet; original unnormalized wet files remain. Initial
Cloud impulse followed ClearBuffers (not a complete phase/pointer reset), and
initial Signalsmith reset also retained an output diffusion buffer. The separately
labeled envelope comparison reconstructs both cores before measuring impulses.
Fresh float32 IRs, not quiet PCM16 quantization, supply decay/density/HF summaries.
See data/sf021/metrics.json for actual values and definitions. Those descriptors
are not a perceptual quality score. No hidden matching of raw fault energy.

**Measured chamber/professional benchmark remains unfulfilled.** The suggested
[Freesound585740](https://freesound.org/people/Nox_Sound/sounds/585740/) author page
confirms CC0,.885s, stereo48k24bit; original download requires login/OAuth. No
original file bytes/hash were obtained, no lossy preview was substituted, and no
proprietary/NC IR was embedded. OpenAIR data listings were examined but their
specific source-license page was not verified, so no substitute IR was imported.
The Freesound page is research provenance, not a fictitious Git-pinned code
or runtime dependency. No Lexicon480L/professional chamber equivalence is claimed.

## One electrical fault and why strong excitation matters

Only returnGain changes: healthy.02, fault24. Interpret fixed Rf1kohm and one
return-isolation resistor changing50kohm→effective41.667ohm (partial short).
These are assumed effective values, not a measured broken commercial chamber.
The same common electronics exist in both configurations:

```
dry/source -> sum -> COMPLETE SAME REVERB -> microphone -> fixed wet output
               ^                              |
               |                              v
               +--64sample delay--soft rail--gain--common conduction/filters
```

Mic signal passes a common200Hz high-pass/4500Hz low-pass. A fixedx100 preamp
and assumed.7V unbiased complementary conduction threshold, referred back to
mic units, give sign(v)*max(abs(v)-.007,0). One variable gain follows that common
conduction stage; two fixed inversions can restore positive return polarity.
Common finite headroom yields:

`return = .15 * tanh(g * sign(v) * max(abs(v)-.007,0) / .15)`.

This continuous ideal class-B/soft-rail reduction is an explicit experimental
operating condition, not a calibrated transistor or typical studio amplifier.
[ADI's class-B explanation](https://wiki.analog.com/university/courses/engineering_discovery/lab_14)
supports the conduction mechanism, not our chosen gains/calibration. Actual device
curvature, noise, thermal behavior and loading are omitted. Software-referred
voltages have not been calibrated to pickup/codec levels.

The threshold uses instantaneous filtered microphone voltage, never RMS, a note
trigger or a tone generator. Near silence its incremental gain is zero. A strong
transient can enter the conducting region and excite a different, bounded loop
state; a fixed linear gain above unity would amplify microscopic excitation too.
Return is injected at the real core input64samples later (1.333ms at48k), not
added as an output oscillator. Actual modeled microphone, speaker excitation and applied
return are captured at full rate in PCM and every37samples in loop_trace.csv.
Mono wet readout has a fixed3x calibration in BOTH states. Common.98 output
containment is numerical safety; all delivered normal renders reach it zero times.

## Observed behavior and proof

- Independent double exponential amplifier reference:75 points, peak error
  2.50e-8. Independent double filter/amp/delay wiring checks actual internal nodes,
  peak discrepancy1.67e-6. This validates the declared reduction, not real hardware.
- Weak.005 pluck: no return conduction, healthy/fault WAVs bit-identical. Strong.15
  pluck and sustained synthetic chord: faulty loop sings after input ceases.
  Late spectral peak~1761.5Hz,~90% power in its seven-bin neighborhood; actual
  healthy/fault guitar WAVs are also delivered. No independent oscillator/noise.
- Zero from startup remains exactly silent at fault gain. A single impulse does
  not cross the filtered mic threshold: healthy/fault impulse files are identical.
  Therefore “strong enough” concerns the mic waveform/buildup, not input peak alone.
- Three rates44.1/48/96k: healthy14s probe late RMS<1e-5, faulty late RMS>.005,
  weak return0, zero output0. Sixty seconds unforced at48k remains finite and
  bounded without output containment. This is a measured finite-duration stress,
  not an unlimited hardware stability/quality proof.
- Disconnecting the same gain at8s recovers;12–14s RMS~2.43e-5. Keeping the damaged
  return connected sustains after unplugging. The loop gain is causally responsible.
- Invalid blocks/prepare/gain requests preserve state; finite extreme containment,
  audible-history reprepare repeatability, silence, predelay and room bounds tested.
  Callback guard records no heap/IO/locks. Fixed64frame processing, no iterative
  solver. Reset reconstructs the core outside audio; it is not live preset switching.
- Native CLI verifies25 PCM WAVs, actual loop diagnostics, matched/raw receipts,
  no safety clipping and fresh-process repeatability. Optional source comparisons
  are independently runnable. Host determinism is not cross-platform bit identity:
  upstream standard-library rand affects initial modulation phase on another libc.

The original core also retains its input silence floor (~3.16e-5) and signed
zero-input state tests in Lp1/Hp1. These affect very small tails; no purely linear
or analog-faithful near-zero response is claimed. The added.007 microphone
conduction threshold is separate and identical in both states. No RMS/note gate
was added to trigger the fault. Changing inherited floors would change the
borrowed DSP and is outside this increment.

The class-B knee and in-loop soft limiter are not oversampled. Nonlinear aliasing
has no quantified rejection bound here; inherited interpolation/modulation can
also create sidebands. No alias-free, noise-floor, physical power/headroom or
universal clickless-switching claim. Founder preferred the original healthy core;
final chamber character and fault musical acceptability remain open.

## Resources and validation

Upstream `_USE_MATH_DEFINES` macro redefinition warnings are retained; no
warning-free build is claimed. Exact commands/logs, original defect receipts,
benchmark context, measured spectra,
sections/layout and source hashes: [data/sf021](data/sf021/README.md).
Pre-edit shared utility/registry3/3. Final **Release6/6(47.84s), Debug6/6(119.38s), poisoned ASan/UBSan2/2(142.55s)**
pass with leak detection enabled. Source-independent checks total152, not hundreds
of thousands of repeated assertions. No exhaustive unchanged historical
circuit suites ran locally. All prior CI steps are retained, with additive native
render/sanitizer/ARM evidence steps. Independent remote status is reported in PR76;
local results never substitute for remote success.

Observed GNU14.2 Release on AMD EPYC9V74: three768000-frame passes per mode,
no concurrent DSP tests/builds, median **0.88123us/frame healthy /0.88355 fault**.
Ranges.84176–.90897/.86927–.92665us/frame. This times the complete native process,
including return electronics and per64frame clock instrumentation; file IO/setup
are outside the call. Clocks/scheduling remain uncontrolled. All25 WAV hashes
match across repeats. It is not WCET, an MCU estimate or a utilization claim.

All sample rates retain the original **66,981,888 bytes of float delay/feedback
buffers**, with the original fixed capacities. M7 borrowed core state
is67,011,904B plus live seed vectors/allocator overhead; wrapper1068B. Host core
is67,012,336B and wrapper1072B. Measured host allocator deltas include overhead
and are distinct from cumulative preparation allocations. Reprepare may briefly
own two complete cores (~134MB). This is a major target/lifetime blocker: no
custom SDRAM allocator, placement, complete target link or fit guarantee exists.
Do not call the current implementation Daisy-ready. Buffer-sizing admission that
preserves the chosen full algorithm/audio is the next separate engineering step.

Actual pinned xPack M7 objects, full conservative process call graph and static
frames are recorded in arm.json. Libm includes bounded sinf/fmod/tanhf and other
upstream helpers; conservative ScaleParam branches include powf even though the
normal input-mix call uses a fixed index. No reachable allocator/IO/lock/atomic
helper. Static frame listings exclude caller/IRQ/context and library-helper stack;
no physical high-water measurement. Engine/bundle/Runtime11040/11288/45224B are
unchanged. No board boot/audio/cycles/noise/latency/SDRAM/flash claims.


Actual M7 sections (bytes; full compiled objects include preparation code):

| object | text | data | BSS |
| --- | ---: | ---: | ---: |
|parable_chamber|15772|0|180|
|Biquad|1856|0|0|
|RandomBuffer|826|0|0|

Known object-code audio frame chain:1392(process)+1144(channel)+544(line)+64
(allpass)=3144B before library helper/caller/interrupt/context stack. This is
static inspection, not a hardware high-water bound. Source-comparison M7 paths
also compile: original Cloud process3488B text, Signalsmith8640B text; the latter
has __atomic_load_8 and reports atomic<double>::is_always_lock_free=false.

Fresh float impulse comparison: Cloud T20-extrapolated RT60 **3.29494s**, retuned
Signalsmith **3.29325s**. Cloud normalized echo-density20ms windows at50/100/200ms
are1.054/1.136/1.028; the normalization is Gaussian exceedance probability, not
quality. Cloud high/low spectral-energy ratio(3–12k /100–3k) falls from−18.36dB
at20ms to−23.94dB at400ms. Full definitions and both candidates are in metrics.json.
Measured48k host allocator deltas: Cloud67032512B, Signalsmith1641056B at the
original compact recipe. These include allocator overhead and differ from sizeof.

## Reproduce from a clean checkout

Linux for interposition tests, C++17, CMake>=3.15, Python>=3.9. Selected core is
vendored; only existing SHA-pinned nlohmann3.11.3 is fetched by normal configure.
Observed GNU14.2,CMake3.31.6; an environment-only offline override used
`-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src`
(the public dependency only, no unmerged DSP). From repository root:

```bash
python3 parable-starter/tools/chamber_fetch.py --cache /tmp/chamber-research
cmake -S parable-starter -B build/chamber-release -DCMAKE_BUILD_TYPE=Release -DPARABLE_CHAMBER_RESEARCH_CACHE=/tmp/chamber-research
cmake --build build/chamber-release --target parable_chamber_test parable_chamber_lab parable_chamber_cloud_candidate parable_chamber_signalsmith_candidate -j2
ctest --test-dir build/chamber-release -R '^(chamber_.*|soundforge_registry|soundforge_registry_negative)$' --output-on-failure --no-tests=error
cmake -S parable-starter -B build/chamber-debug -DCMAKE_BUILD_TYPE=Debug -DPARABLE_CHAMBER_RESEARCH_CACHE=/tmp/chamber-research
cmake --build build/chamber-debug --target parable_chamber_test parable_chamber_lab parable_chamber_cloud_candidate parable_chamber_signalsmith_candidate -j2
ctest --test-dir build/chamber-debug -R '^(chamber_.*|soundforge_registry|soundforge_registry_negative)$' --output-on-failure --no-tests=error
cmake -S parable-starter -B build/chamber-sanitized -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -O1' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build build/chamber-sanitized --target parable_chamber_test parable_chamber_lab -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:max_malloc_fill_size=100000000 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/chamber-sanitized -R '^(chamber_causal_feedback|chamber_listening_cli)$' --output-on-failure --no-tests=error
./build/chamber-release/parable_chamber_lab --input parable-starter/samples/chamber/input_take03_20s.wav --output build/chamber-listening
./build/chamber-release/parable_chamber_cloud_candidate parable-starter/samples/chamber/input_take03_20s.wav build/candidates/cloud envelope
./build/chamber-release/parable_chamber_signalsmith_candidate parable-starter/samples/chamber/input_take03_20s.wav build/candidates/signalsmith envelope
./build/chamber-release/parable_chamber_test parable-starter/samples/chamber/input_take03_20s.wav build/candidates/cloud/envelope_upstream_raw.wav
python3 parable-starter/tools/chamber_reference.py --output /tmp/chamber-reference
diff -r parable-starter/tests/fixtures/chamber /tmp/chamber-reference
# Offline descriptive metrics only; requires NumPy2.3.5, not a DSP dependency:
python3 parable-starter/tools/chamber_metrics.py --renders build/chamber-listening --candidates build/candidates --output build/chamber-metrics.json
python3 parable-starter/tools/check_chamber_arm.py --compiler /path/to/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --output build/chamber-arm
python3 parable-starter/tools/chamber_arm_audit.py --compiler /path/to/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --cache /tmp/chamber-research --output build/candidate-arm
```

Omit the research-cache option to build/test only the selected vendored core.
Fresh candidate processes may contain the original uninitialized-constructor
issue: defect receipts retain the poisoned-allocation reproduction. They are
comparative research, not admitted processors. Keep that distinction when testing.

## Changed files, open criteria and next ticket

third_party/chamber: whole pinned MIT core, exact local patches/rights and source
research. include/src: isolated block adapter, bounded room controls and one common
return circuit. tools/tests/fixtures: full-engine comparisons, native render,
independent law/wiring and M7 evidence. samples: downloadable source/audio/receipts.
CMake/CI/registry(+inventory13→14) expose this specimen only. README/status/ADR/data
separate source choice, implementation and unproven claims.452 prior native,
firmware and fixture files remain unchanged. Open-branch CMake/registry appends
must be reconciled on future rebase; no other specimen entries overwritten.

1. Borrowed healthy source: running, compared and founder-preferred. Final shorter
   chamber musical approval and measured/professional IR benchmark remain open.
2. Single causal gain fault: implemented; amplitude-dependent bounded singing,
   weak identity, no self-start and repair recovery measured. Musical approval open.
3. Focused host/sanitizer/source/ARM evidence supplied; no hardware admission.
4. Seed3 execution and final musical quality explicitly not established.

Next smallest ticket: bound/own the selected core's delay storage for Seed3 while
preserving original-core and audition regression, then measure a standalone
hardware callback. Obtain the cleared original chamber IR for the outstanding
reference comparison. No patch-engine/phone/storage/biometric work is proposed.
