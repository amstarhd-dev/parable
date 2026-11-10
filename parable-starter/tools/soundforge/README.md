# DSP Soundforge — host laboratory and evidence registry

SF-001 shares a small laboratory; it does not prescribe a shared electrical solver
or runtime processor abstraction. LabRAT is the first specimen; SF-002 adds an
experimental CD4049 specimen. See its [report](../../docs/CMOS_REPORT.md) and
[measured reuse log](../../docs/SOUNDFORGE_SF002_WORK_LOG.md).
SF-003 is the third circuit architecture: [germanium report](../../docs/GERMANIUM_REPORT.md)
and [work log](../../docs/SOUNDFORGE_SF003_WORK_LOG.md). It reuses the same facilities
and adds no general transistor solver or runtime integration.

## Facilities

- `analysis.h`: model-independent C++17 whole-wave RMS/peak/mean, finite checks,
  amplitude dB (floor 1e-15 / -300 dB), shared RMS target and explicit matching gain.
  Empty/nonfinite signals and invalid requests reject. Sample accumulation order
  and matching arithmetic are extracted from LabRAT without numerical changes.
- Existing [`../lab_wav.h`](../lab_wav.h): reuse its bounded mono PCM16 parser and
  writer unchanged, including its supported rates/duration/size/error policy.
- `excitation.h`: the owned nine-second harmonic plucks/strums extracted from
  LabRAT, now used by both specimens. Default .15 peak preserves every LabRAT
  byte; CMOS requests .05 model volts. Synthetic excitation is not recorded guitar.
- `check_labrat.py`: deterministic byte comparison to captured main outputs and
  protected sources, not a scientific reference or a generic effect acceptance test.
- `registry.json` and `validate_registry.py`: offline asset/provenance/evidence
  inventory. No loading, downloads, dependency resolution or production-readiness flag.

Utilities allocate/throw and belong on the host, never in the audio callback.
`rmsTarget(signals, requestedRms, peakLimit)` returns one target respecting all
crest factors; it does not modify samples. `rmsGain(rawStats, target)` returns a
linear gain (1 for silence). The caller applies it and checks the final float/PCM
peak. Always retain raw levels, DC, parameters and gains. RMS matching is neither
LUFS/perceptual matching nor a way to hide clipping, instability or model faults.

FFT/coherent spectra, fault selection, stage-response math, SPICE stamps/comparisons
and circuit gates remain model-specific. CMOS imports the existing `run_spice`
process/log wrapper, not RAT equations. No general circuit solver is extracted.

## Registry contract (version 1)

All `paths`, evidence `paths`, source `provenance_path` and `notice_paths` are
regular files relative to the **repository root**, including `parable-starter/`.
No absolute, parent/dot, Windows, empty-component or escaping symlink paths.
Each stable ID has type `primitive`, `model`, `test` or `reference`, behavior,
limitations, ownership/physics notes, major dependencies and all seven evidence
categories. `not-applicable` means the category does not describe that asset;
`not-performed`/`not-integrated` records missing work. Neither means success.

Sources reference an integer `candidate` in the existing provenance file's
`candidates` array, which supplies URL, exact revision and license. `used` means
direct reuse; `derived` means adaptation/derivation; `audit-only` means evaluated
without import. Used/derived entries require local notices and an exact license
label in the conservative import allowlist: **MIT or BSD-2-Clause only**, matching
the currently audited sources. All other labels, including restrictive, unknown,
unlicensed, unreviewed permissive licenses and compound expressions, reject for
import; they may still be recorded as `audit-only`. No automatic aliases, SPDX
expression interpretation or per-record exceptions. Expanding this policy requires
a documented source/license review and an explicit validator/test update. Automated
eligibility is **not legal clearance** or authentication of a license claim.
Candidate arrays must not be reordered without reviewing/updating their references;
stable upstream IDs remain future maintenance work. Existing provenance records the
offline simulator pin and neutral PR #11 lab origin; its generic kernel is absent.
Repository-owned utilities have no external source entries; origin/ownership is
explicit, and the global project license remains **unspecified**.

The validator checks required fields/types/version, IDs/dependencies, source
references, statuses and all declared local files. It does not browse, authenticate
claims, verify copyright ownership or infer evidence from a passing test. The
models explicitly have no real-guitar audition, physical Daisy run or patch runtime
integration. Reduced-section SPICE agreement is not full physical-circuit validation.

## Reproduce

From the repository root, C++17, CMake >=3.15 and Python >=3.9 (standard library):

```bash
python3 parable-starter/tools/soundforge/validate_registry.py
cmake -S parable-starter -B parable-starter/build/sf-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build parable-starter/build/sf-debug -j2
ctest --test-dir parable-starter/build/sf-debug --output-on-failure --no-tests=error
cmake -S parable-starter -B parable-starter/build/sf-release -DCMAKE_BUILD_TYPE=Release
cmake --build parable-starter/build/sf-release -j2
ctest --test-dir parable-starter/build/sf-release --output-on-failure --no-tests=error
./parable-starter/build/sf-release/parable_rat_lab --output parable-starter/build/rat-auditions
python3 parable-starter/tools/soundforge/check_labrat.py --directory parable-starter/build/rat-auditions
./parable-starter/build/sf-release/parable_cmos_lab --output parable-starter/build/cmos-auditions
./parable-starter/build/sf-release/parable_germanium_lab --output parable-starter/build/germanium-auditions
python3 parable-starter/firmware/build.py
```

ARM build requires Linux x86_64/Python >=3.12 and the existing pinned recipe.
CTest includes independent utility tests, positive/negative offline registry
checks and all sixteen original renderer-file hashes. Exact hashes characterize
Linux x86_64 GCC runs, not a promise of bit identity on every compiler/platform.
An unexplained mismatch fails; do not refresh hashes to bless a refactor's sound
change. Investigate, quantify and obtain founder review first.

Release CI keeps the original audition artifact, notices, source/compiler SHA
identity, settings and checksums; it adds the registry and compatibility report.
Interpret registry paths against the recorded source checkout, not the flat ZIP.
`SOURCE_COMMIT` is the checked-out commit; a pull-request workflow may check out
GitHub's synthetic test-merge commit. The PR report identifies the branch head
separately; neither metadata nor a successful test-merge means the PR was merged.

## Add the next model — one recipe

1. Pin and inspect credible licensed DSP and original circuit research first.
   Record exact files/revisions/licenses, failed paths, rejected alternatives and
   transitive gaps. Borrow -> adapt -> independently simulate/model -> invent,
   documenting the actual gap before original DSP work. Never infer rights from
   a README or import evaluated GPL/unlicensed material.
2. Add one specimen record and provenance/notice files, following the six
   current records. Separate code reused/derived from references only consulted.
   Record assumptions, topology, physical parameter units, bounds and omissions.
3. Reuse `lab_wav.h` and `analysis.h`; write the model's own runner and independent
   reference fixtures. Preserve simulator conditions, version, netlists and hashes.
   Do not reuse RAT reference equations as independent truth for another circuit.
4. Run source-independent numerical comparisons and safety/stability/reset/input
   tests. Generate labeled synthetic dry/processed WAVs, retaining raw levels and
   exact controls/gains. Use owned/permitted real guitar when available; founder
   listening remains a distinct gate. Keep unsuccessful experiments in the audit.
5. Run both host configurations and the pinned ARM build. Report object text/data/
   BSS, instance memory, static work estimate and dependencies. Separate compilation,
   firmware linking and actual target timing/analog/boot measurements.
6. Update evidence per category, limitations and acceptance report. Track the next
   model's time/rework/reuse/resource metrics from
   [`SOUNDFORGE_BASELINE.md`](../../docs/SOUNDFORGE_BASELINE.md). Open an unmerged
   PR reporting what reusable knowledge was gained. Permanent patch integration
   requires its own approved ticket; no correspondence rules are invented here.
