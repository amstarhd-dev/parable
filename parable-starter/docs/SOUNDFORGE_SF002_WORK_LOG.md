# SF-002 — observed work and reuse

Start from main `2cc9412c1968d1c229bcbedba7fb6e19dfd6f416`. Compare the historical
[SF-001 baseline](SOUNDFORGE_BASELINE.md), not a reconstructed PAR-007 timesheet.
Actual UTC observations are retained in [sf002-work-events.jsonl](sf002-work-events.jsonl).
They are named gates/manual interventions, **not every command or active labor**.
Local logs/output trees stayed outside Git; CI supplies independently retrievable
proposed-source tests, resources and audition artifacts. No speedup claim.

## Timing and interventions

First recorded clock was **2026-11-10 02:53:25 UTC**, already during repository
setup, not an exact user-request/start-of-labor timestamp. In Toronto this window
began November 9 at 21:53:25. Pauses/tool waits count in elapsed times.

| Observed gate | UTC | Elapsed from recorded window |
| --- | --- | --- |
| Baseline started | 02:53:58 | 00:00:33 |
| Published measured CMOS + MIT code identified | 03:03:32 | 00:10:08 |
| Clean host + ARM baseline passed | 03:04:27 | 00:11:03 |
| Research decision written, before native code | 03:07:35 | 00:14:10 |
| First safety + independent-reference kernel passed | 03:23:15 | 00:29:51 |
| First eight shared-kernel WAVs written | 03:25:04 | 00:31:40 |
| Expanded final Debug/Release gates recorded | 03:40:03 | 00:46:38 |

First-WAV timestamp comes from actual `LISTEN.txt` filesystem mtime, not later
polling. Those eight WAVs are retained and compared byte-for-byte to final local
outputs. First experimental WAV time is **not** the later CI publication time;
complete checkout/compiler/source manifests are attached by CI. Initial kernel
gate predates expanded endpoint/CLI coverage. Gate timing is not completion of
musical acceptance. No active labor duration was tracked, and PAR-007 comparable
timing/step counts were never recorded; two uncontrolled projects cannot prove
productivity improvement.

Manual work actually performed: (1) repository/issue/rules/review audit;
(2) clean host/ARM baseline; (3) eight source/license candidate inspections;
(4) research choice; (5) native adaptation + reduced solver; (6) independent
SPICE stamps/generation; (7) safety/reference gate; (8) shared excitation and
auditions; (9) input/metadata/endpoint regression tests; (10) registry/notices;
(11) final host/sanitizer gates; (12) ARM resources/binary comparison;
(13) clean reference regeneration and reports; (14) PR/CI/artifact verification.
These **14 named categories** are the measurement definition, not fourteen shell
commands, clicks, person-hours, or fourteen eliminated steps. The event log gives
actual observations; PR/CI/artifact category completes only when linked in the PR.

Observed rework: nested ngspice user functions failed expansion, so the independent
expressions were flattened; one Python f-string formatting mistake was corrected.
The native comparison then caught SPICE converging to a nonphysical loaded DC
root outside valid drain-voltage bounds. Physical nodeset guesses and domain
rejection fixed that reference; acceptance tolerances were not loosened. Added
stale-output deletion so the reused simulator wrapper cannot accept old raw files
after failure. Initial audit download handling trimmed upstream EOF; exact raw
bytes were restored before hashing. Compile indentation warnings were fixed.
Tonepad drawing retrieval failed and TI's digital model was unsuitable; both are
explicit research limits. This is an observed list, not a complete defect count.

## What was actually reused

Physical lines include comments/blanks; categories overlap only where explicitly
stated. Reproduce with `wc -l` on the listed files. LOC is not saved engineering
time or evidence of musical quality.

| Asset | Lines | Actual treatment |
| --- | ---: | --- |
| Existing `tools/lab_wav.h` | 83 | Unchanged reader/writer reused by both models |
| Existing `tools/soundforge/analysis.h` | 58 | Unchanged levels/DC/RMS matching reused |
| Existing `tools/soundforge/validate_registry.py` | 152 | Unchanged offline validation and license policy reused |
| Existing `rat_spice_reference.py:run_spice` | 20 | Imported function/process/log wrapper; no RAT equations copied |
| New shared `tools/soundforge/excitation.h` | 31 | Extracted old 23-line owned generator, parameterized peak; two consumers |
| New `cmos_lab.cpp` / `cmos_measure.h` | 104 / 83 | Model runner/settings/reports and direct-bin/reference checks |
| New `cmos_spice_reference.py` | 119 | Model-specific independent stamps/fixtures, shared process wrapper |
| New native tests / CLI tests | 89 / 69 | Model-specific safety/state/limits and actual runner regression |
| New kernel / device / API | 116 / 40 / 59 | Dedicated circuit solve, narrow MIT device adaptation, fixed state |

Existing facilities directly reused: **141 lines PCM/stats**, **152 validator**,
**20 simulator wrapper**; extraction creates **31 additional shared lines**.
New model-specific lab code: **306 lines**, tests **158**, native/API **215**.
These totals exclude fixtures, docs, notices, registry records, build wiring and
their edits. They do not pretend that all Soundforge code was executed inside
the kernel. Test practices, CMake/CI, pinned downloads/ARM reports and legal gates
were reused without a new library or recipe.

Deliberate remaining duplication: FIR coefficient preparation (source lines
41..46, 6 lines) and 4x interpolation/decimation (102..108, 7 lines) adapt the
owned PAR-007 technique. **13 physical source lines** plus fixed FIR state stay
local to avoid modifying protected RAT DSP. This is copied DSP technique, not a
shared Soundforge facility. The 5-line CSV field splitter and roughly 7-line
output-overwrite check repeat small laboratory idioms; reporting/CLI boilerplate
and fixture comparison remain model-specific. No new WAV parser, statistics
helper, FFT implementation, general SPICE solver or registry validator cloned.
No claim that all duplicated effort disappeared. A second actual consumer justifies
sharing excitation; it does not yet justify a generic transistor/circuit engine.

Borrow-first outcome: MIT ACME current/Jacobian was adapted; paper/source facts
were cited. Obsidian AGPL, TI digital model, unavailable Tonepad drawing and
unlicensed supplement listing were not imported. Source SHA/notice accounting is
in `third_party/cmos`. Source-search inspection was substantial new work, not
saved by the facility registry.

## Before/after resources and knowledge

Same ARM pins/flags: new CMOS object **5604/0/0 text/data/BSS**, instance **2128**;
RAT remains **6452/0/0**, instance **10752**. Host instance **2136**. All three
preexisting firmware binary bytes and the RAT object match clean main. Object
text excludes final library/link cost. No hardware CPU, deadline, power, noise or
latency measurement. Full evidence and static operation bounds are in
[CMOS_REPORT.md](CMOS_REPORT.md), including the known internal-codec flash probe.

New reusable knowledge: a traced, MIT-compatible measured CD4049 device model;
the distinction between analog transition models and vendor digital models;
physical-root selection/domain checks in an independent reference; explicit
fit extrapolation and ideal-rail boundaries; all-phase bounded solver accounting;
component-driven coupling/feedback differences; source-identified same-kernel
auditions that reject fallback/rail-clipping before RMS matching. These facts and
fixtures are cumulative engineering evidence, not proof that a musical voice is
accepted. Physical comparison, real guitar, founder listening and integration
remain separate gates. No astrology interpretation was added.
