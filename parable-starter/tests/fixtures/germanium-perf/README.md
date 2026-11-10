# SF003-PERF evidence (observations, not portable timing goldens)

`before-*` was captured on clean main440a99221af7cc3f3b7123db327bb161306d4856,
after PR19, **before numerical/selector source edits**. Environment/compiler/source
hashes and the exact original standalone driver are retained. To compile that
driver: compile original `src/parable_germanium.cpp` normally, compile
`initial-driver/perf_bridge.cpp` with `-DPERF_FACTORY=perf_libm`, then compile
`initial-driver/germanium_bench.cpp`; include original `include/`, `src/` and the
driver directory. Link the three objects. Flags are in before-environment.json;
run `binary 7 128`. The counting build defines PERF_COUNT for the driver and
links `-Wl,--wrap=expm1 -Wl,--wrap=sqrt`, then runs `binary 3 1`.
No instrumentation is used in wall-time trials.

`tools/germanium_perf/baseline.py` exports that original Git source and uses the
present same-workload driver (additional implementations omitted at compile
time). It is a convenient independent baseline reproduction, not a claim that
the revised driver has the original driver's SHA.

Exploratory summaries retain negative results: positive-half inverse symmetry,
signed tables, then a noinline numerical-device boundary. Three trials of32768
frames are screening observations, not the final seven-trial paired benchmark.
The associated numerical sweeps precede the final zero-sign correction. Initial
evaluator revisions are described in the plan; they are not extra production
implementations. The final generator/source is the reproducible candidate.

Final O2/O3 JSON contains every trial plus best/median/worst, host affinity,
source/binary hashes and actual compile commands. Separate count data must never
be used as speed evidence. VM frequency/load variation exists even with affinity.
No timing threshold is asserted by CTest. Error results use the fixed plan's
gates; failed candidates remain explicitly failed and unselected.
The known failed H32/P8 mathematical candidates skip expensive circuit validation
and must not be mistaken for zero-error/validated circuit candidates.

Hash files preserve all original germanium, CMOS and RAT files captured;
these are Linux/x86_64 GCC observations. An unexplained cross-compiler mismatch
must be investigated, never silently refreshed to approve changed DSP.
