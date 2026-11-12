# SF-008 — spring/contact experiment (2026-11-12)

**Standalone experimental sound delivered; founder listening is an open gate.**
Compare `04_healthy_matched.wav` / `05_contact_matched.wav` in CI's
**parable-spring-auditions** ZIP. Also compare `strong_healthy_common64.wav` /
`strong_contact_common64.wav`: these make the actual mechanical crash easier to
hear. Both retain exactly the same offline gain64, as do the weak/impulse pairs.
Raw versions remain alongside them. The PR links the verified artifact/run;
30-day retention, reproducible CI and pinned input below. No human musical
approval, measured spring-tank fidelity or Daisy runtime claim.

[Gate0](SF008_GATE0.md) was committed before native edits; [ADR-008](adr/ADR-008-spring-contact.md)
records the body, physical assumptions, bounds and step-dependent omissions.
Main **a17f64f** includes merged #46. #42 is still open and untouched; no other
branch imported, merged or repaired. Root AGENTS and frozen Lexicon are not
tracked; project AGENTS/Constitution, issue, Soundforge, ADRs/contracts/resource
reports were read. No new correspondence semantics or activation rule.

## Model, evidence and limits

Narrow MIT Rings positive-stiffness modal-placement adaptation, two96-mode paths,
192 total; explicit positive mechanical energy coordinates. Borrowed modal
foundation is **not** a complete helical spring PDE. Different modal frequencies
produce dispersive propagation/ringing; cosine drive/pickup/contact locations,
frequency-dependent material attenuation, force coupling and finite decay are
explicit equivalent assumptions. No generic hall, triggered sample/noise or added
crash delay. Source choices, rejected alternatives and file-level MIT grant are
in [provenance](../third_party/spring/provenance.json). No unlicensed MATLAB or
tank recording imported; Bilbao/Parker2010 consulted, not copied.

Only ONE positive path0 support clearance changes: healthy5mm -> faulty5um.
Motion crossing the stop causes a restitution impulse, e=.55. Total contact
energy decreases even when HF modes gain energy. Numerical position projection
removes additional energy; its loss is reported separately, not presented as a
physical impact law. Unchanged path1 remains sample-exact between configurations
when the common input-port budget is inactive (as in all normal proofs/renders).

Initial20um proposal failed the guitar experiment: peak.25 DI reached only2.201um
positive displacement and produced0 impacts. Even peak.8's healthy7.044um would
not contact20um. Selected5um / common inputpeak.8 gives real DI141 impacts without
port limiting. Weak5ms peak.05 pluck has0 and identical healthy/fault audio; strong
peak.8 gives1689 at48k. The single-frame.8 impulse gives0: it carries much less
energy than the5ms pluck, and is not falsely labeled a strong crash stimulus.

Independent fixtures use60-digit Decimal orthogonal projection/restitution on
three energy coordinates, including outward/inward/clear/negative-clear cases.
A separate continuous complex-exponential force convolution predicts the
**untouched second path**, without native output, discrete splitting, FIR or
contact code; fixture velocities rounded to12 decimal places for reproducibility.
These are independent calculations under shared equivalent assumptions, not
SPICE or measured tank validation. Every unforced frame is checked against
`E(t)<=E0*exp(-6t)`; full signed input/material/contact/projection/clear ledger
is checked against stored energy.

| Rate | Strong impacts in1s | Restitution loss | Projection loss | First impact |
|---|---:|---:|---:|---:|
|32k|1241|8.777nJ|1.054nJ|8.6875ms|
|44.1k|1534|8.167nJ|.794nJ|8.6848ms|
|48k|1689|7.988nJ|.758nJ|8.6875ms|
|96k|2851|6.908nJ|.426nJ|8.6771ms|

Refinement gates: projection drops below60% of32k value at96k; largest HF kinetic
increment varies by<.2%; onset spread<62.5us. Full crash waveform and chatter
counts are **not proven converged or sample-rate invariant**. At48k maximum
penetration.1402um; projection9.49% of restitution loss. Largest impact HF kinetic
increase1.2434nJ, with positive restitution loss. First8192-frame Hann spectrum's
>=2kHz power fraction: healthy3.37e-6, fault.01024; automated gate requires >100x
and fault>.001. These spectral fractions are not joules or perceptual ratings.

Continuous untouched-path reference RMS errors at32/44.1/48/96k:
5.943e-7 /6.939e-7 /3.960e-7 /1.979e-7 m/s; peak<4e-6, finest RMS<.7 of coarsest.
44.1k observation times include nearest-frame quantization. Normal ledger error
<1e-18J gate (observed guitar maximum2.65e-22J), no fallback, port limiting or
nonfinite audio. Tests cover stopped/invalid preparation, atomic rejection,
reset/history/rate changes, dry/wet/return endpoints, silence, extreme samples,
resonant overload and callback guards including actual6s clears.

**Safety construction:** stored energy<=2uJ via an explicit bounded quadratic
input-port budget (not the malfunction or hidden output limiter). Cauchy–Schwarz
pickup bound and193-tap FIR L1 norm bound wet return to.8 normalized, with convex
mix and wetReturn0..1. At most2 contact decisions/frame; fixed2x stepping, no
iterative solve, allocations/free/locks/IO/JSON. Six-second maximum stored tail
after last nonzero input; conservative return bound at clear<1.3e-8, recorded
energy discarded. No physical noise floor claim. Material attenuation scales
position/momentum, a passive split-loss approximation, not an exact viscous ODE.

193-tap Blackman output FIR at2x: cutoff.42*base rate, group delay48 base frames.
Impact discontinuities create bandwidth beyond the finite modes. `alias.csv`
uses16384-frame strong-probe windows at32/48/96k (different physical durations):
fault-output power above16k is4.11e-8 at48k /1.10e-7 at96k. This exposes residual
HF bandwidth and sample-rate tradeoffs; **not measured native folded-alias
rejection or alias-free certification**. Modes stop near8k. No intersample
collision localization, finite-duration contact or continuous analog validation.

## Listening and levels

Permitted pinned GuitarJam/Julian-br CC0 DI, original and public card included;
[notice/ownership limits](../third_party/spring/AUDIO_NOTICE.md). Entire493501
frames/44.1k, software gain4.62335097002 -> peak.8 normalized, equal6s silent tail,
no resampling;758101 frames/file. Not calibrated transducer voltage.

Twenty-one mono PCM16 WAVs: six dry/healthy/fault raw/RMS-matched music files,
nine raw weak/strong/impulse input/responses, six extra processed probe files
with shared offline gain64. Main wet only, mix1/wetReturn1, raw common gain1.
Normal native return is deliberately quiet under its conservative safety bound.
Whole-file RMS matching, not LUFS; all receipts are pre-PCM16 float statistics.

| Guitar version | Raw RMS | Raw peak | Crest | Offline RMS-match gain | Matched peak |
|---|---:|---:|---:|---:|---:|
|dry|.117961|.800000|16.627dB|.600154|.480123|
|healthy|.00116206|.00933977|18.102dB|60.9214|.568992|
|contact|.00116160|.00935096|18.116dB|60.9455|.569899|

Matched whole-file RMS.0707946 (-23dBFS). Raw guitar difference RMS7.332e-5;
weak difference0, strong4.757e-4. No output clipping, numerical containment or
port limiting in any normal render; every body tail cleared once. Music clear
energy about2.30e-25J; strong fault about1.09e-23J. `renders.csv` has levels, crest,
DC, gain, events, losses, tail and independent host timing for every file.
`energy.csv` gives actual displacement/velocity/events/energy terms, decimated128
frames plus **every impact/clear**; rows aggregate two substeps, not individually
resolved collisions. `spectrum.csv`, `comparison.csv`, `settings.json`, `LISTEN.txt`
explain exact experiments. Render/reset repeat is byte-exact per compiler; no
claim of all-platform floating-point bit identity.

## Resources and admission

Actual pinned xPack ARM14.2.1-1.1 M7 hard-float compile, -O2, no fast-math/exceptions:
object text**3392B**, data/BSS**0**; complete processor**18736B** (192x80B modes plus
own FIR/history/probes/config). Hard Gate0 ceilings24576B state/8192B object text
passed. These object bytes exclude math/runtime library link costs. Static frames
process48B/substep152B/prepare152B/reset16B are **not** total transitive stack or
runtime high-water measurements. Source/disassembly/layout/undefined calls in
ARM artifact `spring-audit/`.

Normal audio methods use fixed double loops/FIR and internal calls/memset. The
overload branch contains sqrt/nextafter calls; whole object additionally contains
prepare-side cos/sin/exp/sqrt. Audit explicitly allows those bounded math calls,
rejects allocator/IO/lock/atomic helpers. Never claim there are no library calls.
Separate actual host timed reset pass: GNU14.2 Release/-O3, AMD EPYC9V74 x86_64
container,758101 music frames, healthy2.420us/frame /fault2.439us/frame (~9.37/9.30x
realtime). Excludes prepare/trace/IO/analysis, includes full processor/stores;
one observed pass under shared host load, **not Daisy cycles or a deadline bound**.

Existing Engine11040B/bundle11288B/Runtime45224B and optional RAT10752B unchanged.
This Spring state **does not fit that optional allocation**. No Engine/canonical
registration, pool expansion, storage/living/firmware architecture changes. Future
<=2000M7 cycles/sample admission goal is prospective/unmeasured; not inferred from
host performance. A smaller mode truncation/optimized embedded body would require
its own energy/reference/A-B proof and measured target admission, not hidden here.

Actual unchanged pinned libDaisy1289a000bc29dae1bc0ed4129cfa2df40ef71574 build links
all seven old images, hash-identical to verified SF007 CI: minimal95220B image span;
diagnostic BOOT_SRAM219720B /BOOT_QSPI219720B; RAT95356B; LED51592B; bypass109600B;
RAT-composite109784B. Same official linker layouts, no flash-region enlargement.
Expected non-gating internal-flash JSON diagnostic overflow**88800B** persists.
These are old firmware link proofs, **not Spring integration, bootloader/Seed3
execution or target performance proof**.

## Reproduce / results

From repository root, CMake/CTest3.31.6, GNU14.2, Python3.12.14. Existing JSON3.11.3
hash pin unchanged. Output directories are disposable builds, never source.

```sh
cmake -S parable-starter -B /workspace/work/sf008-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/sf008-Debug -j2
ctest --test-dir /workspace/work/sf008-Debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /workspace/work/sf008-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/sf008-Release -j2
ctest --test-dir /workspace/work/sf008-Release --output-on-failure --no-tests=error -j2
python3 parable-starter/tools/spring_reference.py --output /workspace/work/sf008-reference-repeat
diff -rq parable-starter/tests/fixtures/spring /workspace/work/sf008-reference-repeat
/workspace/work/sf008-Release/parable_spring_lab --input /workspace/work/sf005-research/050.wav --output /workspace/work/sf008-listening
```

Original input URL/SHA in audio notice; CI downloads/hash-checks it. Baseline
**30/30 Debug131.63s /Release87.79s** before code. Full increment **32/32
Debug240.13s /Release113.41s**, then focused spring2/2 Debug221.97s /Release58.51s
after common-gain audio/refinement additions. Times reflect concurrent host work,
not speed claims. Final clean CI repeats full32/32 in each configuration plus
fresh reference regeneration/actual guitar repeat/artifact identity.

Focused ASan/UBSan uses the complete existing19-test boundary set plus both new
spring tests (21); ThreadSanitizer retains all four activation/living tests.
Exact flags/targets/test expressions in [host workflow](../../.github/workflows/host-tests.yml).
Local sanitizer commands use these same expressions and additionally reuse the
unchanged downloaded JSON source via
`-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf008-Release/_deps/nlohmann_json-src`.
ASan flags `-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie`, linker
`-fsanitize=address,undefined -no-pie`, Debug; ASAN_OPTIONS=detect_leaks=1:halt_on_error=1,
UBSAN_OPTIONS=halt_on_error=1. TSan flags `-fsanitize=thread -fno-omit-frame-pointer
-fno-pie -O1`, linker `-fsanitize=thread -no-pie`, TSAN_OPTIONS=halt_on_error=1.
Local ASan/UBSan **21/21,609.81s**, leak detection enabled; TSan **4/4,50.88s**.
Only sanitizer CI timeout grows10->15min because the complete local run alone
exceeded10min. Final Release spring tests2/2,28.89s after all reference/audio checks.

```sh
python3 parable-starter/firmware/build.py --cache /workspace/work/par005-fresh-cache --output /workspace/work/sf008-arm
python3 parable-starter/tools/check_spring_arm.py --compiler /workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --output /workspace/work/sf008-arm-audit
```

Full original fixtures unchanged. RAT16-file golden preservation CTest passes;
CMOS8 WAVs /germanium20 WAVs match retained prior hash manifest. Tape17 and rotary19
deterministic files (including12 rotary WAVs) match previously verified CI ZIPs;
variable host-time `renders.csv` excluded. Canonical/living27 files match fresh
before-edit hashes. All131 preexisting native/firmware/fixture hash entries
unchanged. Registry only adds ninth asset/expected count; no policy/allowlist edit.

## Changed files, reuse and remaining gates

New header/native/contact: bounded standalone body. New lab/reference/fixtures/
safety+CLI tests: same-kernel audio and independent numerical proof. New MIT notice,
provenance/audio notice, ADR/Gate0/report; README/status/registry/CMake and two CI
workflows expose/reproduce it. Existing DSP/Engine/storage/activation/living,
production firmware/pins/linkers and original fixtures are untouched.

**Actual reuse:**259 physical lines of existing unchanged PCM83/stats58/excitation31/
guards87; seven-line FFT copied unchanged, narrow owned FIR window/normalization
idiom adapted. Existing CLI/ARM/CI conventions adapted, not a new framework. New
native/interface/contact about195 lines, model-specific lab/reference/audit213,
safety/CLI tests about175 (physical-line metric, not effort or algorithm fraction).
Rings recurrence/position foundation borrowed; energy/contact body and independent
proof are local gap work. No claimed productivity percentage or48-voice admission.
Baseline log creation18:33:38UTC -> first retained guitar WAV19:02:43UTC =29m05;
this is baseline-to-render elapsed time, not full ticket time or active labor.
First standalone checked executable built19:00:27; exact first pass timestamp and
active labor were not separately tracked. CI records final gate times/source.

Observed rework:20um produced no guitar contact; selected5um and explicit common
peak.8. Initial compiler misleading-indentation warnings corrected; added
convergence assertions needed an explicit<algorithm> include (local build failed,
fixed/rebuilt before delivery). Low raw probe level motivated labeled common64
copies, never replacing raw. Continuous-reference formatting rounded to12 decimal
places to avoid fragile last-ULP fixture text differences. No goldens loosened.
No unexpected final CI failure is accepted; expected JSON overflow above remains.

**Unfulfilled human/physical gates:** founder must judge recognizable musical
spring and convincing crash; no physical tank fit, measured clearance/force,
transducer/coupled helical/gravity/shell/contact-duration model, M7 CPU/cache/stack/
IO/deadline/boot test or patch admission. These are visible open criteria, not
certifications implied by finite samples or a successful ELF.

**Next smallest ticket:** founder listening decision on healthy/contact guitar
and common64 strong probe, with preferred/raw-level feedback. If approved, a
separate borrow-first smaller embedded approximation and Seed3 CPU/state admission
A/B, preserving the full host specimen. No broader plugin/control architecture.
