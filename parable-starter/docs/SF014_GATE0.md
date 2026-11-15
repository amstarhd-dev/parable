# SF014 Stage0 — one loaded Lockhart cell, one upper-feed resistor drift

Issue58 selects Aquarius wavefolding; #57 Libra is parked, not a dependency.
This is a standalone Soundforge research/audition specimen, after pilot essentials.
No Engine/schema/slot/editor/firmware/hardware/living/correspondence work. No other
PR is merged, cherry-picked or edited. Founder retains musical acceptance.

## Baseline and instructions, before final DSP

Fresh clean worktree `sf-014` from current main
`c3aaca164e7a3f7d5b0106b3dd8dc89b7e1ef940`, including independently merged54.
Read project AGENTS, Constitution, README/kickoff/status, Soundforge recipe and
SF009/010/011 reports/ADRs. Read SF012 report/ADR **only as prior-art documents
on unmerged56**, not as imported implementation. Main has36 tests/11 registry
assets. Searches found no Aquarius/wavefolder implementation or duplicate SF014.
Root AGENTS and frozen correspondence Lexicon are absent; no substitutes invented.
Open42 CMOS/48 spring/56 tape are untouched;57 remains parked. No dependency on
their merge. The cloud runtime temporarily detached at onboarding, then reconnected;
no unavailable execution was reported as a successful build.

Pinned local CMake/CTest3.31.6, GNU14.2.0/Linuxx86_64/Python3.12.14. BEFORE edits:

```sh
cmake -S parable-starter -B /workspace/work/sf014-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/sf014-Debug -j2
ctest --test-dir /workspace/work/sf014-Debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /workspace/work/sf014-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/sf014-Release -j2
ctest --test-dir /workspace/work/sf014-Release --output-on-failure --no-tests=error -j2
```

PATH includes `/workspace/work/baseline-tools/bin`. Debug **36/36,1450.69s**;
Release **36/36,264.12s**. Existing source/firmware/fixture hashes and fresh
canonical/living outputs captured for preservation. Existing optional RAT state,
Engine/bundle/Runtime10752/11040/11288/45224B are not expanded. No current M7
execution, physical audio or founder approval is established by this baseline.

## Borrow-first source audit

| Candidate, exact revision | Actual inspection / decision |
|---|---|
| HSU-ANT/ACME.jl `3832ccafb952ee2ca0a27286c091d613062c23fd` | MIT2015/16/17 Martin Holters, LICENSE.md and src/elements.jl:bjt inspected. Adapt reduced Ebers–Moll transport/terminal currents and derivative foundation already used by main. Preserve complete notice. No Julia/framework import. |
| danmohd/LockhartWavefolderJUCE `caa89c683a70eb077dc05806dfa58d92f05c43b0` | Source/Lockhart.{h,cpp}, README/tree inspected; no root code license found. Symmetric analytic Lambert-W/ADAA, no unequal-resistor transistor network. **Audit-only**, no code/coefficients copied or inferred paper license. |
| madskjeldgaard/portedplugins `c6c0d7430e9bc8e8aaeae3db8263e1dbb8f398eb` | GPL3 LICENSE and plugins/LockhartWavefolder/LockhartCell.cpp inspected. Up to1000 Halley iterations and documented exp overflow concern; symmetric cell/cascade. **Audit-only**, no import. |
| annaobara/Buchla-259-Wavefolder `c3f4045e2d09bf367e46a2625ffbd4b3d06edfa5` | MIT2025 Anna Obara LICENSE/src/wavefolder.m inspected. Different five-branch Buchla circuit; polyBLAMP code expects known sine crossing times/fundamental, unsuitable for arbitrary guitar without new estimation machinery. **Audit-only**, not a Lockhart substitution. |

Import policy remains MIT/BSD-2 only. Global repository license is unspecified;
no relicensing inferred. Reuse unchanged main PCM/statistics/excitation, callback
guard/check harness, SPICE wrapper, take03 preparer and owned FIR/FFT/lab/ARM
patterns. No generic solver extracted and no source from open56 imported.

Primary: Esqueda/Pöntynen/Parker/Bilbao2017,
[Virtual Analog Models of the Lockhart and Serge Wavefolders](https://doi.org/10.3390/app7121328),
Figure1/Table1. Publisher PDF
`https://mdpi-res.com/d_attachment/applsci/applsci-07-01328/article_deploy/applsci-07-01328.pdf`,
SHA256`2e9a1465f9190381c402d478fbacb114fdc758656ad006638c80d75d9b3b19b6`.
Paper is CC-BY4.0; consulted as mathematical/physical reference, **not a code
import grant**. The `/pdf` web endpoint returned429; the actual publisher PDF
was downloaded/read successfully. No diagram artwork/prose copied. Our literal
netlist and equations below make polarity/loading verifiable without artwork.

## Selected actual circuit and physical envelope

One cell only. PNP QP upper emitter through RP to+15V, NPN QN lower emitter
through RN to−15V. Bases share ideal buffered input; collectors share C. RL=10k
from C to ground. RP=RN=15k healthy. Ideal unity inverting readout `f(Vin)=-Vc`.
It isolates common output coupling C=100nF/R=100k (10ms,15.9155Hz) from RL.
Ideal sources/buffer are assumptions, not physical Daisy rails or pickup loading.

```
VCC P 0 15
VEE N 0 -15
VIN IN 0 0
RP P EP 15000
RN EN N 15000
RL C 0 10000
QP C IN EP PNP_model
QN C IN EN NPN_model
```

Generic reciprocal Ebers–Moll, transport IS=1e-17A, BF=100,BR=1,T=300K,n=1,
zero intrinsic junction/diffusion charge/terminal resistances/Early/high-injection/
noise. Published ideal αF=1/αR≈0/constant-VBE assumptions are NOT used to cancel
the damaged branch. Finite BF/BR are declared nominal assumptions; no measured
transistor pair/unit is claimed. Modern SI VT=.025851999786V differs slightly
from the paper's rounded.025864V and ngspice's constants. Validate that difference.
The paper supplies topology,±15V/15k and1–50k load scale; our finite-beta law and
10k selection are explicit. Normal guitar cell peaks.2/.6/1.2V; LUT covers±3V
for interpolation/containment headroom, still well within modeled supply domain.

For normalized NPN-orientation junctions use `IF=IS*expm1(VBE/VT)` and
`IR=IS*expm1(VBC/VT)`, αF=BF/(BF+1),αR=BR/(BR+1).

```
QP: VBE=EP-VIN, VBC=C-VIN
QN: VBE=VIN-EN, VBC=VIN-C
KCL_EP = (EP-15)/RP + IFp/alphaF - IRp
KCL_EN = (EN+15)/RN - IFn/alphaF + IRn
KCL_C  = C/RL - IFp + IRp/alphaR + IFn - IRn/alphaR
```

All three equations are solved jointly on stopped control. The special3node
arrowhead Jacobian reduces exactly to one scalar Schur complement; no generic
network solver/API or altered device law. Analytical implicit derivatives supply
interpolation slopes. Runtime uses a prepared representation of this same curve.
The core is memoryless; recovery comes from declared common output coupling and
resamplers, not invented transistor storage, envelopes, pitch tracking or motor/noise.

## Fault candidates and independent evidence before final code

Independent pinned ngspice44.2 builtin PNP/NPN and literal netlist, not native
stamps. Scan−3…+3V and inspect actual slope reversals and saved branch currents.

| RP | f(0), post-inverter V | Lower/upper turnover inputs V | Decision |
|---:|---:|---:|---|
|15000|~0|−.310…−.311 /+.310…+.311|healthy, genuine turnover, not monotone clipping|
|22000|+.793665|−1.746 /−1.072|**selected one fault**; upper turnover remains inside intense−1.2V guitar range|
|33000|+.811161|−2.960 /−2.242|research only; both folds outside intended guitar drive, less useful diagnostic|

Both roots move negative; do NOT force a positive faulty fold or add an artificial
bias. Exactly RP15k→22k, RN/load/rails/devices/buffer/gain/EQ remain unchanged.
The source current imbalance produces the raw collector bias `Vc≈−.793665V`.
Keep that raw curve and currents visible. Common physical output coupling prevents
steady DC reaching the processor output; reset represents a precharged capacitor,
not a synthetic one-sided transfer, state-specific trim or hidden bias correction.

The independent3node prototype matches SPICE nodes within.289µV for all three
cases. SPICE output KCL peak≤.142nA with the current saved-vector setup. An early
probe read scalar `@Q` currents after DC and repeated the last values; KCL caught
this invalid evidence. Explicit `.save all @qp[ic]...` fixed it; no bad current
dataset is treated as validation. Full load/rail/amplitude/harmonic/reference
refinement and final native tests remain required after this selection.

## Cost / alias alternatives, measured before selection

- Paper's symmetric analytic Lambert-W evaluated using a separately derived
  log-domain Newton method. Peak3.883mV difference against our **finite-beta**
  SPICE model (±3V), not evidence of a paper error. Its symmetry assumptions do
  not directly describe RP≠RN. Do not apply it with a post-hoc shift.
- Bounded full3node Newton cold starts:4/8 updates fail5778/5384 of6001 healthy
  points,16 fails632;64 converges all. Fault4/8 fail6001/4843,16/64 converge all.
  Measured isolated direct-cell costs~3.22µs healthy/~1.76µs fault at64, distinct
  from full-processor timing. Keep this accurate control/reference evaluator;
  don't put that worst-case iterative work into audio or disguise failures as sound.
- Cubic Hermite of actual solved values/implicit derivatives, float knots/slopes,
  double integrated-prefix.256/512/1024 intervals use4112/8208/16400lookup bytes.
  Peak SPICE error healthy48.20/3.682/.392µV; fault36.90/2.522/.388µV. **512**
  selected: microvolt accuracy without doubling lookup memory. Integrate the
  actual rounded cubic exactly; primitive and curve must be derivative-consistent.
- Compare1x/2x/4x raw and first-order ADAA, using actual C++ full-processing
  prototype with interpolation/decimation and common coupling,44.1/48/96k,
  healthy/fault, high1760/3520Hz and harsh8kHz tones plus busy high-voiced chord.
  Independent dense SPICE curve(.00005V step) sampled64x/128x supplies numerical
  truth with independently computed linear filters; reference refinement RMS
  ≤5.2e-10V. Chord comparison uses full reference error, not a false pure-tone
  harmonic mask. Frequencies use actual coherent input bins. An initial bin-label
  mismatch/infinite dBc denominator was corrected; spur level is relative to
  total reference RMS so a legitimate fundamental null cannot corrupt the metric.

| Mode | Worst high-guitar3520Hz RMS error | Worst8kHz RMS error | High-guitar spur / harsh spur dBr(reference RMS) |
|---|---:|---:|---:|
|1x raw|18.61%|141.10%|−12.59 /+4.93|
|1x ADAA|16.10%|85.93%|−17.13 /−.83|
|2x raw|1.530%|12.33%|−35.30 /−15.72|
|2x ADAA|2.941%|15.07%|−50.60 /−32.01|
|4x raw|.1922%|2.200%|−54.17 /−31.06|
|4x ADAA|.7365%|3.793%|−78.29 /−60.80|

Choose **4x ADAA**: intentionally trades slightly more wanted-waveform error for
much lower folded-back spur energy. Do not claim it dominates every metric.
Busy-chord RMS error.1074%,1760Hz.2722%. ADAA has half an internal-sample delay;
error comparisons explicitly align that known delay, not arbitrary phase/gain.
No12x/16x assumption or cascade. Host complete prototype state9640B, full-path
4x ADAA measured~.412…1.545µs/frame on contended Linux/GNU14.2 (minimum per
condition comparable to4x raw~.389µs). Final same-class timing remains required.
No inference of target cycles or universal alias/perceptual threshold from this.

## Final implementation envelope and gates

One stopped-prepared, thread-confined native class. Normal4x/193tap symmetric
FIR+ADAA;1x/2x/4x and ADAA-off are explicit scientific diagnostics. One physical
input unit=1modelV, source calibrated.2V peak; Drive1/3/6 gives.2/.6/1.2V cell
peaks. Valid Drive0…6, finite input containment±.25V; table±3V (prove interpolation
L1 bound for arbitrary accepted histories). Output volts/8 common headroom,
not physical ADC/DAC calibration. No normal-render containment or solver fallback.

Selection gates, set here **after feasibility research and before final DSP**:
static peak≤20µV to independent full circuit; fold displacement error≤1mV;
native interpolated collector KCL≤200nA; analytic/direct point KCL≤1nA; first
primitive derivative consistency; same-state silence/reset/NaN/Inf/finite extremes,
transactional rejected prepare, valid/rejected dynamic Drive and44.1/48/96k guards.
Full-path high-guitar/chord relative RMS error≤1%; harsh8k≤5%; unwanted-tone
spur≤−60dBr(reference RMS) at the fixed intense validation probes. Report all
point/peak/harmonic errors, discarded candidates and sample-rate/filter conditions.
These are measured experimental bounds, not universal perceptual or physical proof.

Complete state≤12KiB, object text≤16KiB, no callback exp/log/Lambert-W/solver/
allocation/lock/IO/exception or unbounded work. Bound≤386 resampling tap MACs,
≤8 cubic evaluations/baseframe,4 ADAA divisions plus constant output coupling.
Initial **conditional planning ceiling10000cycles/baseframe** assumes cached
instructions/data/no IRQ/memory stalls,≤12cycles/tap plus generous polynomial/
divide/loop allowance. This is an estimate, **not a measured or certified WCET**;
actual Seed3 throughput and future2000cycle admission remain unproven. Record
pinned M7 object/call graph/static frames/full state and actual host full-path cost.

## Deliverables / musical gate

Reuse unchanged `klon_input.py` and exact Guitar-TECHS take03 original/20s mono16
excerpt SHA`75ae5e73c72e6774fb75e6318dc91ba66347fb65290949e947144033eb648f52`.
Full CC-BY4 attribution, original/archive hashes and exact transform travel with
the artifact. No new or better-sounding take. Restrained/medium/intense dry/healthy/
fault, raw/common-gain and separately RMS-matched files, bounded disclosed gains.
Synthetic sweeps/attacks/long notes/chords remain separate. Explicitly compare
actual same-input RAT, Muff and CMOS renders and their structures/level responses,
not just the word metallic. Human musical judgment and literal sitar recognition
are not claimed by numerical plots. No sixth cell, artificial oscillator/envelope,
post-hoc bias shift, pitch tracker, ring modulation, extra voicing effects or new
architecture. If this one cell is harsh/thin, retain the WAVs and recommend only
a separately reviewed small voicing experiment. One tested, unmerged PR.
