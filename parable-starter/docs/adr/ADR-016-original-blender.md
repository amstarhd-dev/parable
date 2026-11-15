# ADR-016 — original-style four-knob Blender and open D1

Status: selected standalone experiment, issue62. Independent review, founder
listening and target admission pending. Gate0 records sources, actual both-diode
candidate results, exact model assumptions and quantitative gates before code.

Use the documented five-NPN/four-Ge-diode circuit: actual Q1/Q2 gain/bias feedback,
loaded Sustain and680ohm internal preamp dry tap, Q3 collector/emitter splitter,
D1/D2 recombination, Q4/germanium clipper, switched loaded Tone/Level, Q5 make-up
and passive Blend. Factory-style1015/build sources support1.5k/680ohm/25k/2.7nF.
Use electrical divider fractions, not a claimed measured mechanical taper.
Model original loaded SPDT bypass; no perfect phase inverter/rectifier/crossfade.

Adapt main's MIT ACME reciprocal EM foundation, owned stopped passive elimination,
BDF2 stiff-state and FIR/FFT idioms. No generic framework or draft61 source.
Fourteen junction ports preserve all16 signal and14 declared junction-capacitor
histories plus nonlinear diode diffusion charge. Nominal silicon and1N276 model
parameters are explicit assumptions, not measured vintage-unit calibration.
Stopped DC preparation, bounded pivoted Newton, DC-relative coordinates, actual
exponentials. No memoryless whole-pedal substitution or stale block linearization.

Exactly D1 opens: Q4-base common anode to collector-side coupled cathode. Remove
only its current/leakage/charge. Both candidate opens were measured independently;
D1 changes octave dominance and raises tested level. Preserve that in raw/common
and separately RMS-matched files. No output fader/severity/random malfunction.

Compare1/2/4/8x before selecting playback; retain converged direct and independent
SPICE references and every original Gate0 failure.32KiB state/24KiB text are
standalone budgets, not Engine capacity or2000-cycle target admission. Measure
host processor cost and actual M7 object/calls/frames. No hardware execution,
production integration, astrology/preset/firmware/living changes. Founder listening
is a separate required decision; leave the scientific PR unmerged/draft if gates miss.
