# SF003-PERF work/evidence log

Base440a99221af7cc3f3b7123db327bb161306d4856, after PR19. Task start was recorded
at2026-11-10 22:43:40UTC; final local validation checkpoint23:35:29UTC. This is
elapsed session time including builds/benchmarks and concurrent work, not measured
human labor or a productivity comparison. CI/PR work follows that checkpoint.

1. Read issue20, repository engineering/product/status and Soundforge/SF003
   research, ADR, reference and tests. Baseline before numerical edits:
   Debug/Release16/16, actual pinned ARM, all three complete audition sets.
2. Audited original GP expensive operations and compiler math. Benchmark the
   unchanged native processor/device:7 trials,524288 frames/trial per case.
   Separate wrapped calls, no instrumented-time claims. Preserve driver/source
   hashes and original WAV/report bytes before editing.
3. Wrote numerical/work/event gates before approximation results. Tested
   positive-half Hermite32/64 and polynomial8/12 with inverse symmetry: slower
   full processors, H32 mathematical failure. Retain negative observations.
4. Revised signed tables/polynomials, emission-scale reciprocals, then numerical
   noinline boundary after inspecting host solve expansion. Same gates/circuit;
   no stale-value cache, block approximation or tolerances changed.
5. Seven-trial paired-O2/O3 full/device measurements. H64/P12 scientific gates
   pass; H32/P8 fail. No consistent full-processor benefit; leave libm enabled.
6. Reuse independent original SPICE and add four separately solved source/rail/
   gain corners. Pin existing ngspice44.2, preserve original fixtures. Validate
   raw samples, nodes, loading, response, harmonics, recovery and failure work.
7. Final Debug21/21, Release21/21, ASan/UBSan8/8; actual pinned ARM objects and
   disassembly. All54 original WAV/report files byte-identical; generate40
   candidate WAVs with existing lab code. Report limitations and no-go.
8. CI on GCC13 exposed a static-table reference warning through a temporary
   selector lambda. Use an explicit table pointer; preserve all arithmetic.
   Re-run affected Debug/Release/sanitizer checks and CI. All5 ARM objects and
   local O2 benchmark remain byte-identical; O3 output differs, so preserve
   original O3 observations as historical and publish final-head CI measurements.

Actual reuse: same153-line circuit engine, only include/two call identifiers
change. Reuse unchanged118-line native lab renderer,135-line independent circuit
reference script/stamps, shared PCM/excitation/analysis and all earlier model
regressions. Narrow GP equation duplication is one private device evaluator,
not a copied engine, generic solver or new model. Existing toolchain/JSON/ngspice
pins and MIT provenance remain; no new dependency or imported exponential code.

Measured engineering outcome is a reproducible no-go experiment and equivalence/
cost harness. Newly integrated public primitives0, additional BJT consumers0,
hardware integrations0, correspondence changes0. Prospective reuse by other BJT
circuits is unproven. Object/RAM/work evidence is in GERMANIUM_PERF_REPORT.md;
do not turn eliminated expm1 calls into a speculative processor speed multiplier.
