# SF-003 — measured reuse and observed work

Updated-main base `d628ffc3e0beeca40ca06506c4e1344781be4667`, after PR #17 merged;
no unreviewed branch import. Required Soundforge facilities present. Compare the
[SF-001 baseline](SOUNDFORGE_BASELINE.md) and [SF-002 observations](SOUNDFORGE_SF002_WORK_LOG.md).
Those are distinct uncontrolled projects, not a speed benchmark.

## Observed clocks and interventions

First recorded clock: **2026-11-10 12:09:26 UTC / 07:09:26 Toronto**, already during
setup. Exact request/start-of-labor and active labor were not tracked. Events in
[sf003-work-events.jsonl](sf003-work-events.jsonl) are actual observations, not a
reconstructed timesheet. Tool waits/overlap count in elapsed time.

| Gate | Recorded UTC | Elapsed from recorded window |
| --- | --- | --- |
| Clean baseline / identified measured sources | 12:14:36 | 5m11s |
| Written research choice, before native code | 12:18:34 | **9m09s** |
| First independent-reference + safety kernel pass | 12:34:11 | **24m46s** |
| First twenty actual raw/matched WAVs written | 12:37:02 | **27m36s** |
| Final submitted CI | PR description's timestamped CI receipt | Recorded after all submitted workflows finish |

First-render time is actual `LISTEN.txt` mtime, not a later polling timestamp.
Initial/final WAVs and reports match byte-for-byte locally. These are synthetic
files, not human musical acceptance. The early kernel gate predates later added
device endpoint and paired coarse/4x checks. Full source/checkout/compiler identity
is attached by submitted CI, not invented retrospectively at the first render.
The committed event log freezes at handoff; the PR description adds final CI's
completion time, elapsed time, exact head/run IDs and artifact verification. This
keeps the receipt tied to the tested commit rather than changing it to announce
its own future CI outcome. There are12 committed local events (initial clock plus
11 gates/interventions); publication and final CI add two PR receipt events.
These14 logged steps are not a total count of shell commands/clicks or person-hours.

Actually repeated work: source/license/topology audit; clean Debug/Release/ARM
baseline; literature/implementation choice; passive network/native solver port;
independent reference generation; safety/state/edge gates; metrics and raw/matched
rendering; registry/notices; CLI/overwrite tests; sanitizer/full host builds;
ARM resource/binary comparison; reference regeneration; source-identified CI
artifact download/review preparation. The event log records interventions and
gate results, including failed paths. No timing/command-count baseline for
PAR-007 exists, and current tools/dependencies were already cached/installed.

Observed rework, retained rather than counted as successful progress:

- Original Overton URL failed; official Georgia Tech copy located and its
  measurement/simplification limitations inspected. Z² DSP redirected to a broken
  URL. RT-WDF's actual source-header license barred import; absent root metadata
  was not treated as permissive. Whitehead's model-card keywords/network were
  not assumed interchangeable with our exact circuit/reference.
- Initial symmetric junction-step damping caused healthy solver failures. Centered
  derivative checks passed; diagnosis showed reverse collector swing was being
  limited unnecessarily. Forward-conduction growth limiting fixed that error
  with the24-update budget retained. No faulty audio accepted/normalized.
- A coarse96k native grid had390mV peak node discrepancy at fast transitions.
 192k, equal to48k4x internal integration, reduces RMS/peak errors over6x with
  unchanged gates. This is a measured discretization limit.
- Coarse1x8.8kHz/.05V produced29 domain fallbacks in20480samples. A96-iteration
  probe did not fix domain escape; discarded that change. The paired4x case has
  zero fallback. Use a valid.01V alias fixture; disclose the rejected case.
- Mixed float/double initializer lists and indentation diagnostics were corrected.
  A test's conditional-object member assignment did not mutate the selected
  configuration as intended; an explicit reference fixed the harness. Validation
  was not weakened. Test/report argument typing was corrected before acceptance.

## Code reused, adapted and still repeated

Physical lines include comments/blanks. File totals are reproducible with `wc -l`;
they do not measure engineering hours or saved defects. Shared code is unchanged.

| Asset | Lines | Actual treatment |
| --- | ---: | --- |
| `tools/lab_wav.h` | 83 | Existing bounded PCM16 reader/writer, no clone |
| `tools/soundforge/analysis.h` | 58 | Existing raw levels/DC/RMS/peak/gain, no clone |
| `tools/soundforge/excitation.h` | 31 | Existing owned plucks/strums, no new generator |
| `tools/soundforge/validate_registry.py` | 152 | Unchanged source/path/evidence/license policy |
| `rat_spice_reference.py:run_spice` | 20 | Imported simulator process/log wrapper, not RAT equations |
| New lab / measurements / reference script | 118 /91 /135 | **344 model-specific host lines** |
| New kernel / private device / public API | 153 /36 /73 | **262 native/API lines**, including borrowed/owned adaptations |
| New native / CLI tests | 99 /66 | **165 lines**, using existing check/testing practices |

Direct unchanged host utilities total **172 lines**, validator **152**, simulator
wrapper **20**. No new shared primitive or general solver promoted. The36-line
private device file is a narrow MIT ACME/parameter adaptation, not original device
physics or a reusable public BJT asset. Existing CMake, source pins, host workflow,
ARM flags/reports/dependency downloads were reused with per-target wiring.

Remaining duplication is explicit: six FIR preparation lines and seven processing
lines plus fixed FIR state adapt owned prior kernels; a5-line CSV splitter,
5-line coherent-projection idiom and small path/CLI/report patterns repeat SF-002
practice. The Python CLI test adapts the existing PCM/header/determinism/hardlink
test pattern. ARM adds a16-line compile/layout block patterned on CMOS, plus
report/hash/notice wiring. These are not zero-effort reuse or a second parser/FFT/
statistics library. File totals above include them; do not add them again.

SF-002 had306 new model-lab lines,158 tests and215 native/API; this increment has
344/165/262. The additional work is GP/source-loading/current validation, bias/
raw-relative reporting and its bounded solve. Lower observed elapsed gate times
than SF-002 do **not** prove speedup: scope, research difficulty, caching, code
history and missing active-labor observations differ. The evidence of reuse is
the actual unchanged facility calls and preserved regressions, not a percentage.

## Resources and new reusable knowledge

Under identical pins/flags: new GP object **7108/0/0**, instance **2480 ARM /2488
host**. Prior RAT6452/0/0/state10752 and CMOS5604/0/0/state2128 unchanged. Both
objects and all3 existing firmware binaries match the clean main baseline. The
germanium object is not linked/activated. Math-library link cost, target CPU/
deadline/latency/noise/power/stack remain unmeasured. Static counts show substantial
expm1/sqrt cost, so embedded feasibility is not inferred from these small objects.

New knowledge: measured Ge leakage requires more than ideal gain/Vbe; apparent
gain, bias and input loading interact; authentic positive-ground signs and a
direct interstage connection matter; the same fitted sample twice does not imply
a chosen vintage pair. Input impedance and Q2 bias can differ substantially from
common targets. Preserve physical attenuation in a raw-relative artifact, bound
solver work, and separate underresolved numerical failure from a component fault.
Independent fixtures include device currents, input current, bias, full passive
loading and grid convergence, not only an output curve.

The lab gained scientific cases and provenance, not a universal nonlinear engine.
Three architectures now exercise the same research/measurement process. Physical
calibration, actual guitar, founder listening and runtime integration remain
independent later gates, as the assignment's production policy permits.
