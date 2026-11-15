# SF016 — actual reuse, bounded rework and clock milestones

One fixed circuit, no runtime framework, production integration or open61 code.
[Measured file counts/hashes](data/sf016/reuse.json) are reproducible line counts,
not a claimed speedup or musical-fidelity score.

| Category | Measured evidence / what it means |
|---|---|
| Seven unchanged host/test/source facilities |314 source lines, all byte-identical: PCM16, statistics, owned excitation, check/guard and exact take03 preparer |
| Four unchanged comparison kernels, public CPP/H subset |797 lines, linked only for same-input comparison; **not** counted as implementation of the new Blender |
| New native fixed circuit/header/device foundation |288 lines; narrow MIT ACME EM adaptation and source-derived1N276 charge law, not a novel distortion curve |
| New specimen host tools |949 lines; dedicated references/figures/audio/cost/ARM/evidence, using existing facility patterns |
| New physical/safety/PCM tests |196 lines; source-independent fixtures, guard, repeat and boundary checks |

Manual adaptations: main Muff's fixed passive elimination, zero-bias conductance
split, pivoted Newton/DC translation; main TS808's stiff-state BDF2 rationale;
main owned FIR, lab/PCM/FFT/ARM/guard conventions. Electrical topology/pot endpoints/
loading, diode charge and one true open are new per-circuit work. No generic BJT
or graph library was extracted. Complete eligible Blender prior art was not found;
JMK/modern/restrictive simulation substitutes were rejected with source receipts.

Observed machine/Git milestones, UTC audit timestamps (not human labor estimates):

* Clean baseline started~14:00; Debug38/38 took1560.12s, Release38/38 took135.73s.
  Source research proceeded alongside those required tests, with no tracked edits.
* Gate0 committed14:33:47 before native code. Original budgets/criteria preserved.
* Native kernel/runner/evidence committed15:09:56 at50336e8; actual audio uses
  that exact source and verified component/binary hashes.19 voices, each repeated.
* Whole final regressions began after01c1ba4 at15:22; separate logs retain before/
  after results. Long existing TS808/Klon Debug tests dominate the mandatory suite.

These intervals overlap. No controlled previous-task productivity comparison or
claim that all elapsed time is implementation labor is supplied.

Rework retained rather than hidden:

1. A strict shared SPICE wrapper attempt timed out; a dedicated small process
   wrapper owns explicit temperature/saves/span checks and a60s deadline. The
   shared RAT helper itself was not changed.
2. An unconstrained high-frequency raw export filled temporary disk. Only this
   task's temporary trace was removed. Uniform bounded exports/short high-note
   probes preserve the intended measurements; failed/incomplete runs are excluded.
   Old generated listings/objects were reversibly gzip-archived, exact bytes
   verified; old source and published audio ZIPs remain intact.
3. BDF2/Trapezoidal alternatives were compared with the same gates. Trapezoidal
   current ringing and37 misses rejected it; final BDF2's21 misses remain failures.
4.8kHz reference exports were refined.1/.05us; original coarse evidence and the
   residual convergence measurements remain. No criterion moved to accept it.
5. A NumPy scalar JSON serialization and a case-CSV decimal-format mismatch were
   corrected. Final fresh replay matches bias/case/binary fixture bytes exactly.
6. Full host suites run once before and once after stable implementation in each
   configuration. New-model focused checks/mesh experiments were rerun only for
   actual code/reference failures. No repeated late binary-upload experiment:
   the already known repository artifact-branch/release ZIP path is used.

The result provides actual audio and independently checked circuit knowledge but
**does not clear the original scientific/target/musical gates**. The next useful
work is stiff-state/high-note convergence at measured processor cost, followed
separately by founder audition and actual target measurements. No performance
optimization or product architecture is inferred from the facility line counts.
