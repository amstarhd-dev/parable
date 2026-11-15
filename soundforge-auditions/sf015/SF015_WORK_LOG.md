# SF015 work/reuse and failed-candidate record

Base main `af159d833bdd0b0e17ccc59fcf7edfa506ed98f5`. Gate0 committed before
implementation (`dc1d43b`); native implementation source `1120ec2`. Final receipt/
report changes follow separately. No other PR merged/cherry-picked by this job.
Main already included independently merged59. Active engineering hours were not
reliably tracked; test wall times overlap and must not be summed as labor hours.

## Actual reuse

Unchanged production/reference foundation, verified by baseline hashes:

| Reused source | Lines | Role |
|---|---:|---|
| `src/germanium_device.h` |36 |MIT GP current law and derivatives, unchanged |
| `tools/lab_wav.h` |83 |Owned strict PCM16 parser/writer |
| `tools/soundforge/analysis.h` |58 |Owned levels/common/RMS matching |
| `tools/soundforge/excitation.h` |31 |Owned fallback plucks |
| `tests/activation_guard.cpp/.h` |87 |Owned heap/IO/lock interposition |
| Total unchanged foundation listed |295 |Not a percentage of scientific effort |

Owned Muff's passive elimination, DC translation and pivot/voltage-limiting idioms
were specialized to six ports; topology/current stamps were not borrowed from Muff.
The small owned RAT process/log wrapper was adapted locally for300s full-transient
runs, with source unchanged upstream. Exact MIT transistor sources/notices and
measured-device facts are reused. No new GP evaluator, general solver framework,
codec, WAV library, effects registry architecture or runtime integration.

Four old comparator kernels and427 pre-existing DSP/header/firmware/fixture files
were preserved. New stable registry record is13; only expected count changes in its
negative-test fixture. Numerical/physics obligations are specimen-specific; no
claim that sharing IO tools eliminates research or scientific validation labor.

## Rejected paths and rework

- Patented470Ω feed interpretation saturates Q2; source drawings and independent
  bias analysis support470k. Amplified Parts BOM/drawing conflict resolved before
  DSP.56k output is shunt, despite issue's casual series wording.
- OC44 and untrimmed measured AC128 were independently probed. A three-ISC fit
  matches three patent DC examples but is underidentified, not evidence of real
  2N270s. Reject fit-as-authentication; select measured AC128 with explicit limits.
- Newer upstream FZ1 model's separately credited Gert Willmann card has unresolved
  import grant. Earlier MIT variant retunes circuit resistors. Neither imported.
  Swell restrictive simulator and RT-WDF license/framework path are audit-only.
- Whole-circuit memoryless LUT loses blocking/coupling memory; device2D tables
  exceed the state envelope. Exact GP is retained instead of a leakage-free curve.
- Initial overly conservative reverse-junction limiting stalled large changes.
  Conventional voltage limiting permits movement towards conduction. A monotone
  residual search also stalled this stiff system. Final bounded domain-backtracked
  Newton passed the boundary/two-tone/whole-history tests without changing equations
  or increasing24-update budget. Failed numerical output is not published as audio.
- Built-in SPICE cubic reverse-bias continuation fails the2µV DC gate. Exact44.2
  source inspected; separate full-exponential MNA reference added. Both published,
  with the built-in miss retained. Tiny RC conditioning is explicitly bounded.
- Two early aborted transient runs exposed missing span validation; invalid
  extrapolated data was rejected, regenerated, and is not delivered. Subsequent
  90s replay timeouts under two-core contention motivated a bounded300s local
  wrapper; no electrical/numerical tolerance was changed.
- Dense192k cycles added after examining narrow-pulse plots. Whole-history and
  dense-window RMS are checked separately; sparse points are not mislabeled as
  a continuous-time RMS measurement.
- 1x/2x/4x alternatives measured with complete processors.4x misses false-spur
  limit;8x clears it against16x reference with unchanged48-frame filter delay.
  Original full-band gate still misses12 cases; dedicated strict command exits1.
  The known-FIR diagnostic explains the loss but is not substituted for the gate.
- One host trace timestamp carried4x's phase3/4 into8x; corrected to7/8 from frame
  index. WAVs and electrical values were unchanged, with before/after hashes and
  actual producing source/binary receipt preserved. Current runner emits correct time.
- Actual ARM cross-compilation caught the host/ARM uint32_t typedef distinction in
  a counter `max`; an explicit type resolves it without DSP arithmetic changes.

## Evidence and boundaries

Baseline Debug38/38 and Release38/38; full post-change40-test suites, strengthened
final focused tests, ASan/UBSan and existing TSan are in SF015_REPORT. Host resources
are measured; ARM object/layout/calls/local frames are compiled/inspected. No Seed3
runtime or physical pedal measurements. Source/library licenses are scoped; global
project license remains unspecified. Human/founder sound approval pending.

Generated intermediate build/output caches were removed only after preserving
passing logs/receipts, to fit the managed workspace. Pinned inputs, source, notices,
prior task sources/artifacts and final scientific/listening evidence were preserved.
