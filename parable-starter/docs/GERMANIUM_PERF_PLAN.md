# SF-003-PERF — pre-implementation numerical/benchmark gates

Written after the unchanged-main baseline benchmark, before approximation results.
Base440a99221af7cc3f3b7123db327bb161306d4856, after PR19. Original GP device
source remains intact as the libm reference. Compile the same circuit source with
test-only namespace/header selection; do not duplicate or change its solver.
No new cache, block linearization, warm-start claim, physical equation, parameter,
failure mode, oversampling or termination/budget change.

## Candidates and error reasoning

Compare shared immutable cubic Hermite32/64-cell tables on[0,ln(2)/2], and
endpoint-corrected Taylor polynomials of degrees8/12. Constants generated offline
with high-precision Decimal and exact binary64 output. Range reduction by integral
powers of2 covers the GP normalized domain (approximately-415.4..27.7), not an
arbitrary[-40,40] table. Inverse symmetry handles negative reduced arguments;
the function and its derivative come from the same polynomial/rational expression.
The four GP transport/leakage scalings and current multiplications stay in every
iterate. All selection is compile-time, never a runtime callback mode switch.

At normalized z<=-40, expm1 rounds to-1 and a constant derivative0 is the analytic
derivative of that asymptote. Use a C1 blend over[-40,-36] to the approximate
exponential, not a hard derivative jump. A conservative absolute derivative error
bound is1.375*exp(-36)<3.2e-16 there; current/Jacobian effects after the largest
supported saturation/leakage scales are far below the gates below. Explicitly
report relative tail errors instead of hiding them. No physical voltage clamp is
added. Invalid/out-of-private-domain inputs return NaN pairs; GP's original domain
guard remains unchanged. Preserve signed zero and cancellation near0.

No code or coefficient import is needed for these standard interpolation/Taylor
constructions. Existing MIT GP provenance/notices remain. This is private numerical
machinery for one circuit, not a public physical-transistor primitive.

## Gates fixed before observing approximation results

All errors compare raw values, not level-matched signals. Current/Jacobian floors
avoid meaningless relative error at zero/cancellation. Deliberately huge valid
device arguments are mathematical stress, not calibrated physical operation.

| Quantity | Limit |
| --- | --- |
| expm1 value | abs error<=5e-12*max(1,abs(libm value)) |
| Analytic derivative versus exp(z) | abs error<=5e-16+5e-9*abs(exp(z)) |
| Cell/range boundary value jump | <=2e-13*max(1,exp(z)) |
| Boundary derivative jump | <=5e-16+2e-12*exp(z) |
| GP collector/base currents | abs<=1e-12 A+1e-9*abs(reference) |
| All4 GP Jacobian partials | abs<=1e-10 S+1e-8*abs(reference) |
| Raw normal circuit output | peak<=2e-6 V,RMS<=2e-7 V |
| Deliberately extreme supported circuit output | peak<=2e-5 V,RMS<=2e-6 V |
| Internal node trajectories | peak<=2e-5 V normal,2e-4 V extreme |
| DC node bias | peak<=2e-7 V |
| Input-current trajectory | peak<=2e-8 A |
| Frequency fundamental | relative<=2e-4 above10uV; otherwise abs<=2e-7 V |
| Harmonic complex amplitudes | abs<=2e-6 V |
| THD+N | <=.05dB when reference residual RMS>1uV; use absolute errors below |
| Input impedance | relative<=1e-4 |

Require identical configuration acceptance, zero new normal fallback/containment,
and matching per-base-sample failure/containment events for stressful/underresolved
cases. Per-base evaluation-count difference<=8 and cumulative difference<=1%;
keep the exact24-update budget/residual threshold. Cover all original independent
ngspice cases without relaxing a single threshold, healthy/both degraded cases,
gain/leakage/passive/rate/source corners, full-domain/random/zero/tail/boundary
device samples, source loading, spectrum/harmonics, attack and DC/overload recovery.
If a candidate fails, report it; do not loosen gates to select it.

## Measurement and decision

Unchanged-main baseline: clean Debug/Release16/16, pinned ARM, all three old
audition sets retained. Before numerical edits, seven stable trials of both device
and real processBlock were recorded on pinned CPU0,3-vCPU AMD EPYC9V74 KVM,
GCC14.2,-O3,-DNDEBUG,no fast/finite-only math,no LTO.4096-frame deterministic
buffers,64-frame blocks,8192warm frames,524288timed frames/trial;12 processor
and6 device conditions. Separate linker-wrapped builds count actual expm1/sqrt
library calls; counting overhead is not timed as performance.

Candidate benchmark rotates implementation order per trial on identical buffers,
flags,warmup and sample counts. Retain each trial, best/median/worst observed,
checksums,work/failure counts,host/affinity/flags/source identity. Also measure
target-like-O2; this is host timing, not ARM cycles. Timings do not gate shared CI.
Select a candidate only after science passes and repeatable full-processor benefit
is measured, not because its isolated device or call count improves. No magnitude
is predetermined; report distributions and regressions honestly. If none is
beneficial/equivalent, default remains original libm.

Measure actual selected/reference/candidate ARM code/readonly-table/instance costs
and inspect disassembly. Baseline already uses vsqrt.f64 for valid positive roots,
with an exceptional sqrt library branch; do not replace it or imply all source
sqrt operations are expensive library calls. No hardware performance or listening
equivalence/noise-floor claim follows from these software tests.

## Recorded first result / revised experiment (same gates)

The first positive-half/inverse-symmetry implementation regresses full-processor
host timing (roughly20..40% in exploratory runs). H32 also fails function/derivative
gates; H64/P8/P12 satisfy the first sweep. Preserve these negative observations.
Before further results, try signed Hermite cells (shared2/4KiB rather than1/2KiB)
and signed endpoint-corrected polynomials, removing per-evaluation reciprocal
divisions. For the four fixed emission scales, evaluate voltage*constexpr reciprocal
and apply that same chain rule to derivatives. This only changes numerical
evaluation of the same exponent/derivative equations; saturation/gain/leakage
multiplications stay inside every iterate. No other GP algebra or solver changes.
The original libm header remains byte-identical. Re-run the same fixed gates and
rotated paired benchmarks; retain libm if this is not beneficial/equivalent.

The signed experiment improves device-only cost but still regresses most full
processors. Compiler inspection shows the optimized device expanded inside solve
(~12.8KiB host solve vs~6.1KiB libm). Next isolate the numerical device function
with a noinline boundary on GCC/Clang (ordinary portable fallback elsewhere);
no equation/state/solver change. Preserve the signed timing as a negative result.
