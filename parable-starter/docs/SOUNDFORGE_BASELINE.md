# Soundforge baseline — SF-001

This is the first shared laboratory, not evidence that development is faster.
Baseline: main `2ad06c55aa7ee2b78323201bbf158d535d9a76cb`, the founder's merge of
[PR #13](https://github.com/amstarhd-dev/parable/pull/13), including the strict
four-name variation lookup. PR #11's generic DSP remains separate/unimported.
All issue-required files were present and read before edits, including engineering
rules, constitution, LabRAT reports/provenance/tools/tests, host CI and ARM recipe.

## Before edits: observed facts

Fresh Debug and Release configure/build/CTest passed **6/6 each**, using the normal
SHA-pinned FetchContent download (no source-cache override). GCC 14.2.0, CMake
3.31.6, Python 3.12. No hardware or real-guitar measurements were taken.
The unchanged Release runner generated eleven nine-second synthetic WAVs plus
`renders.csv`, `measurements.json`, `frequency-and-level.csv`, `diode-transfer.csv`
and `LISTEN.txt`. Their sixteen SHA-256 hashes were captured **before edits** in
[`labrat-main.json`](../tests/fixtures/soundforge/labrat-main.json), together with
ten protected source/reference hashes, exact source commit and compiler identity.

Existing physical-line counts (includes comments/blanks, not an effort measure):

| Asset | Main lines | Reuse / remaining specificity |
| --- | ---: | --- |
| `lab_wav.h` | 83 | Already neutral PCM16 I/O; retained byte for byte |
| `rat_lab.cpp` | 128 | Statistics/dB/matching extracted; excitation, controls, reporting and CLI remain |
| `rat_measure.h` | 90 | FFT/spectra, faults and circuit fixture comparisons remain RAT-specific |
| `rat_spice_reference.py` | 99 | Offline reduced-circuit netlists/fixtures remain RAT-specific |
| RAT header / implementation / circuit math | 74 / 164 / 36 | DSP unchanged |
| `test_rat.cpp` | 96 | Existing circuit/safety/regression tests unchanged |

PAR-007 resource evidence: ARM object text/data/BSS **6452/0/0 bytes**, excluding
linked math libraries; instance **10752 bytes ARM / 10760 host**. Static estimate
about 520 multiplies and similar-order adds/base sample at 4x; **not target cycles**.
Original firmware images: internal 74008 bytes, SRAM/QSPI 196292 bytes each.
Full-codec internal flash diagnostic has a known 65380-byte overflow; that narrowly
recognized probe outcome is not an unexpected CI failure or total capacity claim.
Exact pins, sections, warnings and assumptions remain in the existing reports.

Manual LabRAT work included candidate/license inspection, transitive AudioLib
license rejection, topology reconciliation, independent ngspice installation/
netlists/fixture extraction, C++ port/validation, synthetic rendering and level
reports, Debug/Release/sanitizer checks, ARM resource extraction, CI artifact
download/hash inspection and listening preparation. These steps are documented,
but their labor durations, iteration counts and duplicated laboratory LOC were
**not recorded**. No retrospective speedup or developer-hours estimate is made.
Recorded rework includes strict variation-name rejection and repeated-output
upstream mkdir repair; this is not a complete defect count.

## This increment: gained and unchanged

Shared assets: named host-only statistics/dB/RMS-target/gain functions, the existing
PCM16 helper, independent utility tests, a three-asset evidence/provenance registry,
offline structural/path validation with negative fixtures, deterministic LabRAT
before/after checking, and one next-model recipe. Registry references the original
provenance/notices rather than copying upstream revision/license text.

No new circuit, runtime solver, processor boundary, patch integration, physical
testing or correspondence semantics. No FFT extraction. Raw metrics and exact
gain reports remain visible; RMS matching is not perceptual loudness matching.
The byte-regression gate compares **all sixteen original files** in both host
configurations. CI also attaches a compatibility report to the original audition
artifact; no WAV binaries enter Git. See the PR for final run/artifact links.

Observed local results: fresh Debug **10/10**, Release **10/10**; the independent
C++ utility target checks 41 assertions and the Python suite exercises six groups
of catalog fixtures. All sixteen outputs match exactly in both configurations.
The full pinned ARM command passes; `rat.o` and all three firmware `.bin` files
are byte-identical to the retained PAR-007 build. Object/state sizes remain as
above. The local ARM report records base SHA plus a dirty worktree and explicit
source hashes; remote CI supplies the clean proposed-commit identity.

The validator establishes local structure and file existence, not truth of claims
or legal clearance. Global Parable licensing is still unspecified. Exact-output
hashes cover the observed Linux/GCC platform; cross-platform drift must be
investigated/quantified, not silently accepted by changing the baseline.

## Metrics to record on the next distinct effect

Use commit IDs, start/end UTC timestamps and named gate results; distinguish
elapsed time from active labor, recording labor only if actually tracked.

| Metric | Definition for the next ticket | LabRAT historical baseline |
| --- | --- | --- |
| Time to first audition | Ticket start -> first reproducible shared-kernel WAV with source/config identity | Not recorded |
| Time to checked kernel | Ticket start -> named independent-reference and safety gates pass | Not recorded |
| Shared vs duplicated lab code | Count physical lines of reused utilities and new model-specific tooling separately; identify any copied helper and why | File counts above; no historical duplication estimate |
| Tooling steps | Record actual commands/manual interventions, including audits/download/reproduction failures | Categories above, no timed/count baseline |
| Defects/rework | Log observed failures, cause, fix commit and repeated gates; separate DSP from tooling | Two examples above, incomplete count |
| Host/ARM resources | Object/library/instance deltas under identical pins/flags; static work vs actual target timing separately | Sizes above; target CPU not measured |
| Musical/physical acceptance | Synthetic, real guitar, founder approval, hardware run/measurements and patch integration separately | Synthetic only; no real guitar/hardware/integration |

No second model has used Soundforge; productivity improvement is **unproven**.
Next experiment is one architecturally different circuit family through this same
recipe, with its own source audit/reference and founder listening gate. Deeper
shared nonlinear/filter/transistor abstractions depend on that evidence.

## Exact local reproduction

Run from repository root; change only output directories to avoid overwriting a
previous comparison. Before-edit commands used `sf001-before-*`; final fresh
commands use `sf001-after-*`:

```bash
cmake -S parable-starter -B /workspace/work/sf001-after-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/sf001-after-Debug -j2
ctest --test-dir /workspace/work/sf001-after-Debug --output-on-failure --no-tests=error
cmake -S parable-starter -B /workspace/work/sf001-after-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/sf001-after-Release -j2
ctest --test-dir /workspace/work/sf001-after-Release --output-on-failure --no-tests=error
python3 parable-starter/tools/soundforge/validate_registry.py
python3 parable-starter/tests/test_soundforge_registry.py
python3 parable-starter/firmware/build.py --cache /workspace/work/par005-fresh-cache --output /workspace/work/sf001-arm
```

CTest includes the original `--self-test`, generic target without RAT dependencies,
registry CLI and malformed/negative fixtures, and fresh default CLI renders checked
against main. The portable single recipe and API/format constraints are in
[`tools/soundforge/README.md`](../tools/soundforge/README.md).

Unproven validation is preserved: historical factory revision/1N914 calibration,
complete LM308/JFET saturation/headroom, supply/bias/external 30 pF compensation faults,
physical pedal traces, real guitar, founder acceptance, Seed3 boot, actual CPU,
latency/noise/power and runtime stack high-water. Existing reduced-section SPICE
fixtures are consumed, not regenerated by CI. Host checks cannot close these gates.
