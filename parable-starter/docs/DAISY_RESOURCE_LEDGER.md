# Daisy Seed3 resource admission — reference vs production
**Audit at main `767a3c4`, 2026-11-19.** This is an **inventory and engineering gate**, not a claim that the 21 voices are production-ready; many exist only in open PRs.

## Budget
Seed3 480MHz; 48kHz; 48 frames/callback: 480,000 clock cycles/block nominal. **Entire graph** <=70% of block = 336,000 cycles as an initial design reserve (not full ISR WCET). Provisional *isolated* allocations: drive 20%, reverb 12%, delay/mod 5%, all subject to later whole-graph measurement and audible gates. These allocations may be rebalanced based on actual tested patches.

**Physical evidence**: PAR009 mono bypass max 3,357 cycles (0.70%); Gain -> RAT(4x) -> LPF max 370,915 (77.27%), 30,000 valid callbacks each, unconnected input, at 480MHz/48k. This is *body-only* timing; does not include SDK/ISR tail and cannot bound signal-dependent worst case. The first-boot newlib-nano `%llu` format bug prints `max_valid_ratio_ppm=lu` and corrupts subsequent labels; use the raw cycles. RAT currently **fails** total 70% target. These are the only physical effect-callback results in this audit. Host times below are *not* target CPU estimates.

## All 21 models / branches (none admitted)

| Priority | Effect | Branch / status | Strategy | Evidence / next action |
|---|---|---|---|---|
| P1 | rat | main | optimize_reference | DWT composite 370915/480000; 4x FIR/soft-double; 4x97 and mixed-precision M7 bench |
| P1 | muff | main PR50 | optimize_reference | 4x ten-port Newton; ~6-12us host/frame; profile solve, exponential, sparse/float approximation |
| P1 | ts808 | main PR52 | optimize_reference | 16x circuit solve; ~16us host/frame; costed 4x/8x physical-surrogate comparison |
| P1 | klon | main PR54 | optimize_reference | 12x nonlinear solve; ~9-12us host/frame; cached/state-dependent nonlinear approximation |
| P2 | germanium | main | optimize_reference | 4x expm1/sqrt solving; no target cycles; device-table/float and stressed M7 bench |
| P2 | cmos | main; PR42 open | needs_measurement | 4x; untested target cycles; isolated M7 benchmark |
| P1 | scorpio | main PR67 | bench_first | 4x fixed-cost hybrid; ~0.55us host/frame; integrate target benchmark then composite |
| P1 | wavefolder | main PR59 | bench_first | 4x fixed-table ADAA; ~0.5us host/frame; measure M7 cost |
| P2 | libra | open PR70 | bench_first | 4x piecewise model; ~0.26us host/frame; rebase and test M7 |
| P2 | ds1 | open PR69 | needs_measurement | 8x fixed work; ~1.2us host/frame; M7 and high-note comparison |
| P2 | fz1 | open PR61 | optimize_reference | 8x nonlinear solve; ~5-11us host/frame; reduce solver and M7 stack |
| P3 | blender | open PR63 | research_hold | 8x; high-note gates fail; ~22-57us host/frame; repair high-note numerical reference first |
| P2 | tape_saturation | open PR56 | optimize_reference | nonlinear magnetic model; ~10us host/frame; reduced state/float physical model |
| P2 | tape_medium | main SF006 | needs_measurement | ~768kB long-lived tape storage; SDRAM/caching and M7 throughput |
| P2 | rotary | main SF007 | bench_first | bounded work, ~4.4kB state, no callback libm; M7 benchmark |
| P2 | spring | open PR48 | needs_measurement | spring crash target cost unknown; ARM budget audit and bench |
| P2 | taurus_plate | open PR72 | bench_first | ~147kB buffer; ~0.13us host/frame; ARM memory placement and bench |
| P1 | gemini_rooms | open PR74 | bench_first | 43.4kB buffer; ~0.023us host/frame; first M7 reverb/composite bench |
| P3 | cancer_cloud | open PR76 | memory_blocked | 67,011,904B core; dual reprepare ~134MB; hold until reduced memory core same-audio verification |
| P2 | leo_hall | open PR77 | needs_measurement | ~475kB; double-heavy; ~0.316us host/frame; M7 soft-double/SDRAM benchmark |
| P1 | virgo_tiled | open PR80 | bench_first | 48.5kB buffer; ~0.066us host/frame; first M7 reverb/composite bench |

**Scorpio / Aquarius and Gemini or Virgo reverb** are the first *candidate* integrated real-time chain. They are not yet proven on Seed3. Keep reference audio and circuit model for every optimized nonlinear effect and test healthy/damaged settings, high notes, silence/attack/tail, aliasing, and audible difference.

**Critical special case:** Cancer Cloud uses ~67,011,904 bytes of state, leaving only ~96,960 bytes in the Seed3's 64MiB SDRAM before any other allocation; temporary double-core reprepare (~134MB) exceeds capacity. Do not integrate unchanged.

**Distinguish** main tape medium/delay (large buffers, lower-complexity streaming) from the SF012 nonlinear tape *saturation* model.

## Full admission checklist
1. Grounded branch identity, provenance and license; pass reference/unit/sanitizer tests, verified target cross-link.
2. Active ARM firmware measure DWT **with input**, several controls and healthy/fault, and explicitly synthetic stress/upper notes; no missed deadlines, headroom rule and verified clock/block budget.
3. Validate RAM/SDRAM/QSPI *placement* and preset/switching *lifetime*, flash, DMA, cache variability, and measured stack high-water; no audio ISR allocation/locks; verify that the actual boot scheme works.
4. Pass founder musical listening A/B against **unchanged reference model**; quantify alias/harmonic/phase changes. No automatic relaxation of numerical/sonic gates.
5. Re-measure a **representative mixed patch** with drive + reverb + delay/mod routing and worst-case input; **entire graph** under 70% measured callback body with additional real interrupt/IO safety margin.

## Ordered engineering plan
- First make **Scorpio or Aquarius + Gemini rooms or Virgo tiled** work on Seed3 and benchmark separately, then together; solve analog audio I/O before calling it musically accepted.
- RAT: retain 4x/193 reference; test 4x/97 FIR and float32 runtime independently; 1x experiment on [draft PR81](https://github.com/amstarhd-dev/parable/pull/81) is for *cost measurement only*.
- Muff: profile 10x10 solver versus expm1/oversampling on ARM. Host linear lookup-table approximation previously **increased Newton iterations**; do not prematurely ship it. Preserve DC/fault memory in every surrogate. Try float Jacobian and consistent approximations, then target benchmark.
- Klon/TS808 next; then other nonlinear voices. Cloud stays blocked until buffer design meets memory budget and preserves sound.
- Do not treat cross-compilation, a host timing, a unit test, or a simulated benchmark as Seed3 real-time approval.

## Source links
[Main code](https://github.com/amstarhd-dev/parable/tree/main/parable-starter) · [Muff report](https://github.com/amstarhd-dev/parable/blob/main/parable-starter/docs/SF009_REPORT.md) · [Klon](https://github.com/amstarhd-dev/parable/blob/main/parable-starter/docs/SF011_REPORT.md) · [TS808](https://github.com/amstarhd-dev/parable/blob/main/parable-starter/docs/SF010_REPORT.md) · [Gemini room PR74](https://github.com/amstarhd-dev/parable/pull/74) · [Virgo PR80](https://github.com/amstarhd-dev/parable/pull/80) · [Cloud PR76](https://github.com/amstarhd-dev/parable/pull/76).
