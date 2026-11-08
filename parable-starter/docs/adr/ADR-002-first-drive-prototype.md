# ADR-002 — First drive prototype

Status: **PROPOSED — founder musical review pending** (PAR-006).

## Candidate and boundary

Evaluate one generic asymmetric antiparallel-diode-inspired overdrive, with
continuous drive, tone, bias/asymmetry and output level. Musical objective:
clean-ish soft playing at low drive, compressed/rough attacks at high drive,
useful pick/level response, controllable brightness. This is a laboratory kernel,
not an accepted production topology, named-pedal clone or component-faithful model.
PAR-006 explicitly authorizes this experiment; founder approval is still required
before the topology/control semantics are integrated in PAR-007.

Keep the JSON v1 closed schema, native Engine, patch/preparation API and astrology
rules unchanged. The lab calls the shared portable kernel directly. Lab parameters
are invocation settings, not a second saved patch authority. No live activation,
parameter automation, ESP32 transport or storage is added.

## Proposed signal path and physical assumptions

80 Hz input high-pass -> 6 kHz pre low-pass -> gain (1 + 39 drive²) -> 4x FIR
interpolation -> static asymmetric diode curve -> FIR decimation -> two cascaded
tone low-passes (1.2–8 kHz) -> 20 Hz DC removal -> level -> final ±1 safety rail.
Input is bounded to ±4; diode drive coordinate to ±16. These are numerical rails,
not claims about analog voltage/headroom. Coefficients/tables and parameter clamps
are prepared off audio; configure resets state and requires stopped processing.

Physical inspiration: Shockley's diode equation I = Is (exp(V/(n Vt)) − 1.
An ideal memoryless resistor-fed antiparallel pair gives the normalized equation

    u = v + k [exp(v/aPositive) − exp(−v/aNegative)]

Here k = 0.025, aPositive = 0.18 (1 + 0.35 bias), and
 aNegative = 0.18 (1 − 0.35 bias). These are *chosen dimensionless voicing values*,
not measured diode/R/temperature values. Bias changes the two exponential knees;
it is not an injected DC voltage. At bias zero the curve is odd. At nonzero bias
it creates even harmonics and waveform DC, removed by the output high-pass.
No diode capacitance, op-amp rails/slew, circuit impedance interaction, charge
storage, temperature or feedback-network dynamics are modeled.

A fixed 1,025-float table samples the unique monotonic solution over u ∈ [−16,16],
step 1/32. Configuration uses 48 bounded double-precision bisections per entry;
processing uses bounded linear interpolation, with exact zero at the center.
This keeps exponentials/iterative circuit solving out of audio. Table/interpolation
error is measured separately from alias error. It is not a SPICE validation.

Sources for the equations and FIR method (mathematics, no code copied):

- [MIT 6.012, Lecture 6](https://ocw.mit.edu/courses/6-012-microelectronic-devices-and-circuits-fall-2009/resources/mit6_012f09_lec06/), forward-biased p-n junction I–V characteristics. OCW materials: CC BY-NC-SA 4.0; no lecture text, diagrams or code imported.
- [Julius O. Smith, Window Method](https://www.dsprelated.com/freebooks/sasp/Window_Method.html), windowed-sinc FIR design.

Implementation is original C++; no third-party DSP implementation or commercial
model is imported. References supply mathematical background, not copied code.
The repository has no project LICENSE file: project-owner licensing remains
unresolved, and libDaisy's MIT license does not license Parable's own sources.

## Aliasing, cost and alternatives

Use explicit 193-tap Blackman-windowed sinc filters, cutoff 0.10 cycles per
oversampled sample (0.4 times base rate), normalized DC gain 1; interpolation
scales by 4. Polyphase interpolation evaluates 193 coefficients/base sample;
decimation evaluates another 193. Both real filters are present. Nominal FIR
cascade delay is 48 base samples; IIR group delay is additional and frequency-
dependent. Fixed state/tables need no heap. The FIR pair costs 386 coefficient
MACs/base sample, plus four table evaluations and the base-rate filters; plausible
for investigation on Cortex-M7, but not a measured target CPU budget. A lab-only
1x reference bypasses the resamplers to measure the tradeoff with the same curve
and base-rate filters. It is not the recommended audition mode.

Alternatives: tanh/rational waveshaping is cheaper but has a weaker diode-law
connection; a live nonlinear circuit solver is more flexible but adds iterative
cost/convergence questions. Deferring oversampling would be simpler but leaves
avoidable aliasing. This fixed FIR/table design is deliberately one narrow
experiment, not a general oversampling framework. Linear interpolation and finite
stopband rejection still produce errors; measured residuals gate quality claims.

## Acceptance gate

Functional/measurement and ARM compilation results will be recorded in the lab
report, with exact commands and limitations. WAVs must come from this same C++
kernel, with synthetic input explicitly named; real dry guitar is an optional input.
Generated files are listening material, not evidence a human approved the sound.

Founder must listen at matched level to dry/low/high drive, tone and bias examples;
judge attack, decay, cleanup with input level, harshness/fizz, bass definition,
useful control ranges and unwanted artifacts, then accept/revise/reject this
candidate in writing. A later real-guitar audition is also required. No hardware,
pedal-grade, circuit-fidelity or musical-approval claim is authorized by passing
host tests or producing WAVs. PAR-007 integrates only the accepted design with
explicit format migration; PAR-008 then uses a sourced founder-approved Mars trace.
