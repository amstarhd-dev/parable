# PAR-006 — experimental drive listening laboratory

The intended result is something to **hear**: one native asymmetric diode-inspired
overdrive, proposed in [ADR-002](adr/ADR-002-first-drive-prototype.md). It has not
received founder musical approval. Started from main
`bb0635032bfae7b15a2e59de152040bf0a7dcb07` after PR #9 merged. Existing unmodified
host suites passed Debug 4/4 and Release 4/4 before changes.

## Create and hear the auditions

From the repository root, C++17 and CMake >=3.15:

```bash
cmake -S parable-starter -B parable-starter/build/drive-release -DCMAKE_BUILD_TYPE=Release
cmake --build parable-starter/build/drive-release -j2
ctest --test-dir parable-starter/build/drive-release --output-on-failure --no-tests=error
./parable-starter/build/drive-release/parable_drive_lab --output parable-starter/build/drive-auditions
```

Open the WAVs in any PCM WAV player, or import them as separate mono tracks into
a DAW for A/B. The Release job in **Host tests** publishes the same files as the
**parable-drive-auditions** downloadable Actions artifact, retained seven days.
The PR links the passing run and artifact. Download/extract that ZIP to listen;
GitHub does not supply an inline audio player for Actions artifacts. No WAV binaries
are committed. Generated files remain separate from source.

| File | Drive | Tone | Bias | Kernel level | Intended comparison |
| --- | ---: | ---: | ---: | ---: | --- |
| `00_dry.wav` | — | — | — | — | Clean source, no DSP |
| `01_low_drive.wav` | .05 | .65 | .25 | .6 | Clean-ish setting |
| `02_high_drive.wav` | .9 | .65 | .25 | .6 | More gain, saturation/compression |
| `03_high_dark.wav` | .9 | .05 | .25 | .6 | Tone change against high drive |
| `04_high_negative_bias.wav` | .9 | .65 | −.8 | .6 | Asymmetry change against high drive |

Default input is explicitly **synthetic guitar-like plucks/strums**, nine seconds,
48 kHz, 432,000 frames. Six pitched harmonic stacks and deterministic amplitude
 envelopes imitate a pluck/strum gesture; they are not a recorded guitar, pickup,
cabinet or physically modeled string. Attack and release envelopes avoid artificial
source tail discontinuities. Output is mono PCM16, no dither.

All five whole-file RMS levels are matched to **−22.01059 dBFS** in the recorded
local render (a −22 dBFS target reduced slightly to satisfy shared 0.89 peak
headroom). Raw RMS, per-file post gain, final RMS/peak/mean and exact controls/rate
are in `renders.csv`. No LUFS/perceptual-loudness equivalence claim. Highest peak
is dry at **−1.0122 dBFS**; the kernel's final rail is not relied on to normalize
files. CSV levels are before PCM16 quantization; independently decoded WAV RMS
values agree within 2e−6 amplitude, and quantized peaks are <=0.89002 (half-LSB
rounding above the 0.89 floating-point target). No latency compensation: FIR contribution 48 samples plus additional
frequency-dependent IIR delay. Repeating the same local build produced byte-identical
WAVs; cross-platform/libm/compiler byte identity is not claimed. A silent source remains silent; −300 dBFS in the
report is a numerical zero/floor sentinel, not a noise-floor measurement.

## Render a real dry guitar recording later

Use the **same executable and unchanged DSP source**:

```bash
./parable-starter/build/drive-release/parable_drive_lab --input /path/to/dry-guitar.wav --output parable-starter/build/real-guitar-auditions
```

Supported ingress: little-endian RIFF WAVE, **mono signed integer PCM16**, one
16-byte PCM fmt chunk, 32,000 / 44,100 / 48,000 / 88,200 / 96,000 Hz; at most
30 seconds, 6 MiB and 64 chunks. No stereo, float, 24-bit, extensible WAV or sample-
rate conversion. Errors reject explicitly. Chunk lengths, RIFF/file size, alignment,
byte rate, data length, padding and duplicates are checked before sample use.
External input is marked user-provided with unverified recording provenance; the
program cannot certify that a file contains a guitar. Keep the original recording
for reproduction. Include trailing silence: output length equals input length and
any effect tail beyond it is truncated. It never edits the input file.

## Shared native kernel and boundaries

`include/parable_drive.h` / `src/parable_drive.cpp` are the only production drive
kernel. Both the renderer and tests link `parable_drive`. WAV/synthetic/FFT code
lives in host tools, not in the kernel. A slow double exact-solver path is used
only for reference measurements, never for rendering auditions. No Python or
WebAudio replacement DSP exists.

`Drive::configure(rate, parameters, mode)` is stopped-control-side preparation:
finite normalized drive/tone/level clamp to [0,1], bias to [−1,1]. Nonfinite controls,
invalid mode and rates outside [32,000,96,000] reject without changing old history.
Success resets history and rebuilds the table/coefficients. Construction/configure
must stay off audio; table building performs 98,400 exponentials plus FIR trig.
No live/zipper-free parameter automation is claimed. `process` and in-place-capable
`processBlock` use fixed storage; nonfinite input returns zero without advancing
history. Input ±4 and drive-coordinate ±16 rails bound extremes. Near-zero state
is flushed below 1e−20; a defensive nonfinite-output guard resets history/mutes.

The architecture and approximation details are in ADR-002. This kernel is absent
from the native Engine and closed JSON v1 processor registry. Parameters here are
lab invocation settings, not a permanent patch document or second authority.
No correspondence mappings, patch format, preparation semantics or firmware audio
callback were changed. PAR-007 owns integration/migration after audition approval.

## Actual measurements and limits

The lab's `--self-test` and audition render both run coherent sine measurements
through the same C++ kernel at 48 kHz, 8,192-point FFT, after two FFT periods of
settling. Alias fixtures use input peak .4, drive .85, tone 1, bias .5, level .6.
The measurement sums positive-frequency energy outside integer harmonics that
fit below Nyquist, normalized to the measured fundamental. With a single coherent
sine these bins contain folded higher harmonics plus numerical residuals. This
normalization compares at matched fundamental level, not unmatched raw amplitude.
DC and Nyquist bins are excluded; FFT is independently checked using a pure
coherent sine with known amplitude. A >15 dB reduction is the regression gate.

| Coherent input Hz | 1× residual dBc | 4× residual dBc | Reduction dB |
| --- | ---: | ---: | ---: |
| 4,107.421875 | −25.1605 | −58.4298 | 33.2693 |
| 5,876.953125 | −20.7042 | −49.8327 | 29.1285 |
| 8,794.921875 | −13.4102 | −42.4327 | 29.0225 |

The 1× reference uses the same curve and base-rate filters, bypassing only the FIR
resamplers. Both actual 193-tap interpolation and decimation filters are present
in 4×. These results show a reduction, **not alias elimination** or a universal
quality bound. The high-frequency residual could be audible. No multitone/IMD,
full rate/parameter/frequency sweep or analog circuit comparison was performed.

Additional results (`measurements.json` contains full precision):

- At 1,001.953125 Hz, input peak .1, tone 1, bias 0, level .6: low/high drive
  .05/.9 THD+N **−54.9143 / −15.0088 dBc**. Nonfundamental energy changes at
  matched fundamental, demonstrating saturation rather than mere output gain.
- Bias .8 at high drive produces a second harmonic **−21.9188 dBc**; the symmetric
  case is at the numerical-zero sentinel. This is a DSP symmetry check, not a
  physical distortion-floor measurement. Steady biased-output DC is gated <1e−5.
- Tone 1 versus 0 at 5,876.953125 Hz, input peak .005, drive/bias 0, level .6:
  bright output fundamental is **24.1826 dB** higher.
- Max absolute 1× full-path difference against an independent per-sample double
  implicit-diode solver and double filters: **6.00634e−5**, gate <3e−4. Includes
  lookup interpolation and float filter/coefficient errors; not circuit fidelity.
- Long-run constant-input rejection, exact silence/mute, nonfinite rejection,
  extreme/bit-pattern inputs, state recovery, finite parameter clamps/rejection,
  exact per-sample/block/in-place/reset repeatability, level sensitivity/compression
  and zero processing allocations are exercised by `drive_safety`.

Host Debug and Release now run **6/6** (the existing four plus `drive_safety` and
`drive_lab`). Additional AddressSanitizer + UndefinedBehaviorSanitizer drive/lab
checks passed **2/2**. Exact local commands and CI links are in the PR. These
sanitizers execute host code, not ARM instructions. Generated WAVs have also been
read/checked independently with Python's standard PCM WAV reader.

## Actual ARM evidence

```bash
python3 parable-starter/firmware/build.py
```

Same pinned xPack 14.2.1-1.1 compiler, Cortex-M7 Thumb/FPv5-D16 hard float,
C++17/O2, no exceptions/RTTI/fast-math. Actual `drive.o` Berkeley text/data/BSS:
**2,576 / 0 / 0 bytes**. This relocatable object includes kernel/control methods,
not linked libm; undefined symbols are `cos`, `exp`, `sin`, `memset`. Trig/exp are
configuration-only. No allocation/throw/I/O dependencies were found in the object.
A separate compiler layout probe measures **5,912 bytes per ARM Drive instance**,
including curve/FIR/history, not zero working RAM. Host instance: 5,920 bytes.
Direct compiler stack frames: process 48, block 24, configure 112 bytes; not a
call-chain/IRQ or runtime high-water estimate. Reports, object/header, `.su`, layout
probe and build-input hashes are retained in the existing ARM artifact.

All three PAR-005 firmware images remain byte-identical to the prior baseline:
minimal 74,008-byte internal image; fuller SRAM/QSPI diagnostics each 196,292 bytes.
The internal-flash full-codec diagnostic still overflows by 65,380 bytes. Drive is
**object-compiled, not linked into these audio images, deployed or executed on ARM**.
No Daisy CPU/callback cost, target fit with all future effects, hardware rate,
latency/noise, heap/stack high-water, electrical behavior or real-guitar quality
follows from these builds. The FIR pair uses 386 coefficient MACs/base sample,
plus four table interpolations and base filters; target profiling remains required.

## Evidence status and next decision

| Stage | Evidence |
| --- | --- |
| Implemented | One portable nonlinear kernel, host renderer and tests |
| Host-tested | Numerical/behavior/alias/reference tests and real PCM WAV generation |
| Host audition material | Five downloadable synthetic comparisons generated; human listening approval not recorded |
| ARM-compiled | Actual Cortex-M7 object and layout sizes; existing firmware builds preserved |
| Hardware-tested | **No** |
| Real-guitar-auditioned | **No recording supplied or real guitar audition performed** |
| Pedal-grade / founder accepted | **No — ADR-002 remains PROPOSED** |

No project LICENSE exists in the repository. Original kernel code/math provenance
is recorded in ADR-002; do not confuse upstream dependency licenses with a license
for Parable. Project-owner licensing remains an open question.

Next: founder listens to dry/low/high, tone and bias files and records accept/
revise/reject plus preferred parameter ranges. Then PAR-007 integrates only an
accepted drive into a revised versioned descriptor/format/preparation path with
migration tests. PAR-008 follows with one sourced founder-approved Mars fixture,
rule trace and audible A/B. No astrology transformation is invented here.
