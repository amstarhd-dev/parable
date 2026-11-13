# ADR-010 — TS808 feedback-diode experiment

Status: **experimental, founder listening and independent review pending**.
Issue51 authorizes this standalone voice, not correspondence activation or pedal
admission. [Gate0](../SF010_GATE0.md) precedes code and pins one exact drawing.

Use the ElectroSmash TS808 true-bypass/effect-on signal path. Preserve both
nominal NPN emitter followers and all loading,9 signal capacitors, frequency-
dependent non-inverting feedback gain, feedback diodes, active Tone and Level.
P1=500k logarithmic, mapping `(20^Drive−1)/19`; P2=20k linear electrical fraction,
P3=100k linear. This is an explicit pot-law assumption, not measured taper.
Retain C1=20nF, R6=51k, C3=47n/R4=4.7k, C4=51pF and postclip1k/220nF.
Thus ~720Hz selective feedback gain and ~723Hz lowpass physically create mid
emphasis, with active Tone modifying it. No arbitrary mid-hump waveshaper.

Borrow MIT ACME reduced diode/BJT and dominant-pole finite-gain/GBW foundation,
main's owned pivot/companion/FIR patterns and unchanged shared laboratory tools.
No compatible full TS GPL implementation imported; no generic circuit framework.
Specialize one fixed linear network, exact prepared passive elimination and a
bounded coupled solve of transistor junctions, feedback pair and both amplifier
states. Exact electrical pot endpoints, no arbitrary tiny resistor substitution.
Trapezoidal external capacitor companions; backward Euler for stiff internal
op-amp states to avoid nonphysical alternating integration ringing. Validate
against independent continuous ngspice and refine the integration grid.

NPN/diode assumptions and model omissions are explicit in Gate0. Reduced op-amp
drive rate is `clip(wp*(A0*error−u),±slew) − gRail*(u−clip(u,±3V))`, wp=2π30,
gRail=1e9/s; output=4.5V+u through75Ω. This extends the finite-pole model inside
the coupled feedback solve with current-limited charging and stiff rail restoring,
not a post-output asymmetry or tanh. Finite5MΩ input load, ideal9V/4.5V rails;
no detailed IC transistor/current-limit/offset/noise or supply-sag model. Manufacturer
legacy typical numbers at±15V do not certify this reduced9V JRC4558 model.

Healthy contains D1 (inverting→output) and D2 (output→inverting). Fault is a **true
open D2**, its current and derivative absent, not merely high resistance/leak.
Other feedback paths, devices, controls and gains are identical. Pre-code SPICE
finds unchanged DC bias, larger positive excursions bounded by the modeled rail,
and asymmetric transient output. Musical usefulness and exact harmonic changes
must come from measured WAVs/references; no second fault or imposed decoration.

Fixed4x FIR processing, stopped preparation/reset, explicit numerical-domain,
iteration/input/output containment counters. Ordinary evidence requires zero
fallback or safety clamps. One input unit=1modelV; fixed jackVolts/4 headroom,
not measured Seed3 electrical calibration. New distinct realDI take and controlled
balanced sine/triangle/attacks retained raw and offline matched with disclosed gains.

Do not integrate into Engine/canonical/ROM/pools/activation/storage/firmware.
Keep old outputs/fixtures and other PRs intact. State/text budget8KiB/16KiB,
actual pinned M7 compile/call/layout and static work audit, full host regression,
focused ASan/UBSan and existing TSAN. No measured target cycle/physical/subjective
claim. Next gate is founder audition; Sagittarius/Klon is separate work.
