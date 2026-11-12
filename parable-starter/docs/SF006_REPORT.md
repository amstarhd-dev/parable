# SF-006 — listen to an incompletely erased tape loop

Experimental **reduced magnetic-medium** processor, outside Engine. No physical
tape/Seed3 test or founder musical approval. Start from main **0def50b** (merged
#40); #42 was open and was neither merged nor imported. [Gate 0](SF006_GATE0.md)
and [ADR-006](adr/ADR-006-reduced-tape-medium.md) precede native edits. Missing
frozen Lexicon v1.0 is explicitly reported; no correspondence policy is invented.

## Listen first

The PR links CI's **parable-tape-auditions**, retained 30 days. Download/unzip:

1. **04_healthy_matched.wav / 05_weak_erase_matched.wav** — same full clean DI
   guitar, healthy/fault, shared RMS -23 dBFS. Then 01/02 for common-gain raw.
   00/03 are dry raw/matched. No resampling, cabinet, extra ghost layer or noise.
2. **10_AB_input.wav** — A from the first second of that recording; different B
   from halfway through it, written at 2, 4 and 6 s. Same ordinary write head.
3. **11_AB_healthy_wet.wav / 12_AB_weak_erase_wet.wav** — feedback **zero**,
   wet only, 2 s tape. At **2.3 s** B starts replaying; only the defect retains A
   beneath B. At 4.3/6.3 s A weakens further. These use identical common gain.
4. **13_B_only_weak_erase_wet.wav** is a separately processed B-only control.
   **14_A_ghost_isolated.wav** is 12 minus 13 after 2.3 s, raw gain, for inspecting
   A's actual contribution. That subtraction is **analysis only**, never a signal
   supplied to or mixed by the processor. File 12 is the actual faulty output.

Musical files: **44.1 kHz, 705,181 frames / 15.9905 s**, including 4.8 s tail.
Proof files: **44.1 kHz, 441,000 frames / 10 s**. All mono PCM16. Frame zero is
input/transport startup; playback follows the integer head delay (13,230 frames)
plus causal filter response. No timestamp retiming or compensation in playback.

Input: GuitarJam / Julian-br **CC0-1.0**, pinned
`2d467bfec90af19301b01123494f4b2ba64c5a3a/guitar_jam/test/050.wav`.
[Rights/source card/hash](../third_party/tape/AUDIO_NOTICE.md). Described by its
author as clean Strat/Scarlett Solo DI; source/ownership not independently
authenticated. Audio permission is separate from DSP's MIT code-import policy.
CI includes the unchanged original and notices. Default synthetic CLI input is
labeled synthetic, not a substitute for the delivered actual-guitar recording.

## Reproduce from clean checkout

Repository root, C++17/CMake >=3.15/Python >=3.9/curl:

```sh
cmake -S parable-starter -B build-tape -DCMAKE_BUILD_TYPE=Release
cmake --build build-tape -j2
ctest --test-dir build-tape --output-on-failure --no-tests=error
mkdir -p build-tape/input
curl --fail --show-error --location https://huggingface.co/datasets/Julian-br/GuitarJam/resolve/2d467bfec90af19301b01123494f4b2ba64c5a3a/guitar_jam/test/050.wav --output build-tape/input/050.wav
echo '22e7b845fc2d71200c72c13fedbeb5f8ac593e48a01f99875f857e0cc5dfecd0  build-tape/input/050.wav' | sha256sum -c -
./build-tape/parable_tape_lab --input build-tape/input/050.wav --output build-tape/listen
python3 parable-starter/tools/tape_reference.py --output build-tape/reference
cmp parable-starter/tests/fixtures/tape/reference.csv build-tape/reference/reference.csv
```

No recording download in DSP, ordinary tests or firmware. Custom input must be
permitted mono PCM16, .1–24 s; explicit output required. Existing WAV reader and
writer retain their format/error limits. CLI rejects overwriting the source or
hardlinks through every output name. Renders refuse clipping/nonfinite data.

## Actual mechanism and mathematical proof

One cyclic float array stores **normalized remanence**, not a feed-forward ghost
recording. At fixed .1905 m/s tape moves **erase -> write -> play -> erase**.
Write cell w; erase (w+E)%N; playback (w-D+N)%N. Default N=1.2 s, D=.3 s,
E=.015 s, and E+D<N. Each material cell sees erase E samples before write and
play D samples after it. Integer rounding is explicit: E=662/720/1440 at
44.1/48/96 kHz. `medium.csv` identifies actual cells, sample/time, write pass,
pre/post-erase magnetization, retained state at write, new record and playback.
The write pass counts physical revolutions; a mixed-generation cell has no
single original-information age. The A/B experiment separately measures A's age.

- **Erase:** m_e=r*m_old. Healthy r=0, ideal complete erase. Defect **r=.85**:
  insufficient erase field exposure leaves 85% of prior remanence. This is a
  dimensionless reduced efficiency, **not a measured coil current**, open-winding
  diagnosis or spatial head-alignment model.
- **Write:** m_new=.4*m_e+.6*S(4*x_lowpass+f*y_replay). The same finite write
  fraction .6 applies to both states. It represents partial domain/depth
  reorientation. A perfect write would itself erase all old information, so
  deliberately retaining state at write is essential and explicit.
- **Coloration:** MIT DaisySP rational SoftClip, record 4.5 kHz/replay 6 kHz
  one-poles. Regenerative feedback enters before record saturation, **after input
  drive**, f=.65 for music, 0 for proof. Mix=.5 music / 1 proof. No wow/flutter
  in this fixed-speed discriminating experiment; no stochastic state.

This is a phenomenological population approximation, not a complete magnetic
hysteresis model. HyperPhysics explains erase/bias/history; Lalbahadoersing et al.
(1999) describe incomplete overwrite/depth residuals. Those sources **do not fit
or validate .6/.85**, a particular tape or audible fault severity. Source URLs,
content hashes, exact software pins and rejections are in
[provenance](../third_party/tape/provenance.json). CHOWTape's GPL full-hysteresis
model is audit-only. MIT DaisySP primitives were narrowly adapted; no wholesale
dependency, GPL code, curve fit, schematic graphic or third-party DSP binary.

Independent **50-digit Decimal** reference uses chronological write history and
scheduled playback, not the native three-head indexing implementation. It checks
healthy/fault and fault+feedback at all three required rates, wraps/head boundaries,
7 medium/filter values per row: **13,356 values, peak error 5.91839e-8** normalized.
The committed CSV/manifest regenerate byte-for-byte; no native output is expected
truth. Tiny Decimal tails underflow to zero on binary64 comparison, well below
the 2e-6 absolute gate. This validates the stated equations, not physical tape.

With feedback zero the surviving old phrase gain is **(.4*r)^pass**. Native
two-sine A/B and separate real-guitar A/B controls test this directly:

| A age after B overwrites | Healthy A RMS | Fault A RMS (guitar) | Measured A gain |
| --- | ---: | ---: | ---: |
| 1 revolution | 0 | .0179298202 | .339999988 |
| 2 | 0 | .00609613865 | .115599992 |
| 3 | 0 | .00207268713 | .0393039969 |
| 4, no new phrase | 0 | .000704713616 | .0133633588 |

`ghost.csv` includes the B-only energy and A projection/correlation (~1), not just
a nonzero signal. The first ghost is ~-17.1 dB relative to B's wet RMS. Separate
healthy impulse/feedback checks distinguish repeats at D from once-per-loop
retained information at N. Nine-turn tests cover healthy/.85/.95 retention,
different B and geometric decay with feedback zero; healthy A gain <=1e-7.

## Levels, spectrum and costs

Common software input gain **1.44479717813** makes DI peak **.25 normalized**.
This is not guitar voltage or codec calibration. Music statistics before PCM:

| Signal | Raw RMS / peak | Raw DC | Offline matching gain |
| --- | --- | ---: | ---: |
| Dry | .0382208884 / .25 | -4.11862e-5 | 1.85224838 |
| Healthy | .0518286925 / .272657514 | -.0001073924 | **1.36593410** |
| Fault | .0579801294 / .315393001 | -.0002022169 | **1.22101450** |

Matched RMS **.07079457844 (-23 dBFS)**; peaks healthy .3724322 / fault .3850994.
Matching is offline whole-file RMS, not LUFS or perceptual approval. Raw gains
remain one; no matching enters DSP. `renders.csv` records all gains/means/peaks.
`ghost-spectrum.csv` is raw isolated A's Hann-windowed 8192-bin spectrum, not a
generated tone or claimed noise floor. `spectrum.csv` measures coherent first-pass
tones at 375/3000/9000 Hz, two levels. At .25 input/9 kHz, the third folds to
21 kHz at **-39.7362 dBc**; at .05 it is -67.3405 dBc. This exposes aliasing,
not total alias rejection. No oversampling or physical head-gap calibration.

Representative full-processor host pass: 705,181 frames, **8,665 / 8,907 us**
healthy/fault, including output stores, excluding preparation/stats/IO. Timings
vary under host load and deliberately differ between repeated receipt files;
audio, geometry, electrical and spectral data repeat exactly. No x86-to-M7 cycle
estimate, target WCET or claimed speedup over earlier nonlinear models.

Hard memory ceiling: **192,000 cells / 768,000 B** externally owned, Tape <=192 B.
At max 2 s: **352,800 / 384,000 / 768,000 B** at 44.1/48/96 kHz. Default 1.2 s:
211,680 / 230,400 / 460,800 B. Host object **128 B**, actual ARM **120 B**;
no per-cell age buffer or second hidden delay. Reset/prepare clears O(N) on
stopped control only. No copying/moving processor aliases; caller retains storage
until audio quiesces. Invalid prepare does not mutate working state. NaN/Inf input
becomes silence while transport advances; finite extreme input saturates safely.

Inductive |m|<=1 follows from bounded record S and convex write; filters/mix are
convex too. S has slope <=1, so with allowed r<=.95/f<=.8 the maximum retained+
feedback gain is **.4*r+.6*f<=.86**. Zero-input histories decay, not unbounded
regeneration. Tests exercise all three rates, max/min geometry, a 96-turn saturated-endpoint zero-input decay, drive/feedback/
retention endpoints, high/invalid input, wrap, reset/stop and sample-rate reprepare.
Actual guarded sample calls show zero allocation/free/locks/IO. No live setters.

Pinned GCC14.2.1-1.1 actual Cortex-M7 compile: **1292/0/0 B text/data/BSS**,
`process` direct compiler frame **8 B**, prepare **56 B**, reset **16 B**.
Process has **no external calls**: fixed three logical cell reads/two writes,
two one-poles, one rational saturation/division and head-wrap branches. exp/lround/
memset exist only in preparation/reset paths. Artifact contains object, assembly,
layout, source hashes and command log. No target cycles measured. Admission ceiling
**1000 cycles/sample** remains a future hardware gate, not a successful benchmark.

## Integration gate and preservation

**No canonical processor registration or Engine integration in this PR.** The
four-bundle ARM Runtime stays **45,224 B**, Engine 11,040 B. Four max tape media
need 3,072,000 B; existing pool and on-chip regions are not silently enlarged.
External SDRAM is declared by the existing linker/SDK but actual Seed3 access,
cache/MPU, ownership/retirement and timing are unproven. No placement/allocator/
bootloader/framework workaround is added. The finished host sound is independent
of that gate. Smallest follow-up: an explicit external audio-buffer ownership/
placement decision and measured single-buffer Seed3 test before any versioned
canonical registration. No live-switching/4-slot tape claim.

The unchanged full firmware recipe is rerun: seven existing ELF/bin images link,
with original pinned libDaisy/linkers and expected reported internal-codec probe
overflow. These images **do not contain Tape**. Existing code/patch/activation/
journal/living/firmware sources remain byte-identical. Current-main canonical and
living outputs (27 files) repeat; RAT's 16-file golden is never refreshed. Legacy
CMOS **13/13** and germanium **25/25** render/receipt files match preexisting
8f94368 captures whose DSP/runner sources are identical on current main. Seven
actual firmware binary hashes and their 29 source hashes match the verified
PAR-012 CI archive. No old golden/size metadata was refreshed. No new physical measurement.

## Checks, files and unresolved evidence

Baseline **26/26 Debug/Release** before edits. Full local **28/28 Debug (206.69 s) / Release (81.86 s)**;
**17/17 ASan/UBSan (160.95 s)** with leaks enabled; unchanged **4/4 TSAN
(112.53 s)**. Later endpoint-test/registry-only extensions are retested under
Debug/Release/ASan; CI runs the exact final tree and outcomes/artifacts are
recorded in the PR. No sanitizer suppression. Exact local paths:
`/workspace/work/sf006-{Debug,Release,asan,tsan}`, all source from this branch.
Full commands are the clean commands above with Debug/Release build type.
For targeted sanitizers use the exact committed
[host workflow](../../.github/workflows/host-tests.yml): 17 ASan/UBSan tests with
leak checks and the unchanged four activation/living TSAN tests. ARM:

```sh
python3 parable-starter/firmware/build.py --cache /workspace/work/par005-fresh-cache --output /workspace/work/sf006-arm
python3 parable-starter/tools/check_tape_arm.py --compiler /workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --output /workspace/work/sf006-tape-arm
python3 parable-starter/tools/check_activation_arm.py --living --compiler /workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --output /workspace/work/sf006-activation-arm
```

Substitute writable paths; compiler/dependency pins/notices are unchanged.

Changed files: new `parable_tape.h/.cpp` (standalone medium), `tape_lab.cpp`
(actual audio/proof/receipts), `tape_reference.py`/fixtures (independent model),
`test_tape.cpp`/CLI (science/bounds/guards), `check_tape_arm.py` (actual compile),
`third_party/tape` (source/recording rights), CMake/workflows (additive checks and
downloads), Soundforge registry and its explicit specimen-count regression and this report/Gate0/ADR/README/status. Existing
DSP/reference/Engine/patch/storage/living/firmware production sources untouched.

Measured engineering reuse: **259 existing physical lines** across PCM (83),
analysis (58), excitation (31) and guard header/implementation (12/75) are reused
unchanged. **Seven FFT lines** are copied unchanged. The new DSP header/source
are **49/61 physical lines**, lab 153, independent reference 58, ARM audit 50;
these counts are source size, not equivalent effort or a productivity ratio.
No controlled engineering-speedup or person-hour claim.

Reuse: four existing host facilities (PCM, analysis, synthetic excitation, callback
guard) used unchanged; the seven-line owned FFT is narrowly copied to avoid pulling
RAT circuit/fixture APIs or moving protected code. MIT circular/one-pole/SoftClip
idioms adapted, not a new solver. Original work is the erase/finite-write medium
and discriminating experiments; no claim that a licensed full defective-tape model
was borrowed. No new dependency or evidence-registry license/status exception.
Rework: initially including the whole RAT measurement header pulled its unused
fixture macro; narrowed FFT reuse instead. Decimal tails required underflow-aware
comparison. Exact half-sample erase spacing uses double 15 ms, avoiding a binary32
tie rounding down at 44.1 kHz. No scientific acceptance gate was relaxed.

Unproven: actual tape/head erase efficiency and overwrite, hysteresis/bias/minor
loops/depth/track physics, wow/flutter, physical saturation/noise, real IO voltage,
Daisy boot/SDRAM/cache/CPU/deadline/stack/heap/ISR behavior, Engine integration and
founder musical acceptance. A playable file and numerical agreement are not those
proofs. Next: founder A/B listening and independent review; no automatic merge.
