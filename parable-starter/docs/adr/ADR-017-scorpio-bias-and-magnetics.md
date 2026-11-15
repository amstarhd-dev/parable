# ADR-017 — Reduced Skywave-family transistor/transformer voice

Status: experimental; founder sound approval and runtime admission pending.
Scope: SF-017 issue66. No correspondence rule or Engine change.

Use Aion FX Skywave1.1.0 (2024-01-21), schematic p7, at9V. This is an
accessible adaptation of Hudson Broadcast, not an authenticated original or a
certified clone. Q1 is nominal2N5088 silicon NPN; Q2 nominal2N404A germanium
PNP. Select gain-switch center-off, trim0Ω, C1 omitted/C2=1µF. The charge pump
is outside this core. Gain is the actual5.6kΩ+250kΩ feedback branch; Low Cut
is Q2 emitter degeneration, including C8/C9, not a post-distortion EQ.
A1Ω Low Cut wiper/contact resistance is an explicit numerical/physical assumption.

Choose a stopped5-node Ebers–Moll solve to prepare513 nonuniform Hermite knots
and their consistent primitive. Retain assumed1kΩ input-source loading, actual
DC supply resistors and leakage, and linearize the capacitor/feedback network
at each prepared bias. Runtime applies its five-state bilinear filter followed
by the prepared nonlinear curve,4x FIR and first-order ADAA. Nonlinear transistor
charge dynamics, signal-dependent output impedance, Early effect and measured
transistor spread are omitted. C4/C6 are guitar-band AC shorts; supply is AC
stiff. This is a bounded reduced physical family, not general transient MNA.
Nominal parameters/independent reduced SPICE netlists are in
`tests/fixtures/scorpio`; sources and license are in`third_party/scorpio`.

Retain C10, transformer flux, finite source/load and the820/1070Ω winding DCR
from Triad TY-141P's2019-05-31 sheet. Matching impedance10kΩ is not winding
resistance. Assume1:1, L0=20H and flux scale0.008V·s:
`i(phi)=phi/L0+phi^3/(L0*scale^2)`. This passive anhysteretic magnetic law has
positive energy and decreasing differential inductance with flux. Winding
voltage drives flux; magnetizing current loads the transistor source. Three
fixed scalar Newton updates solve the backward-Euler cubic at each internal
step. Neither hysteresis, remanence nor measured saturation onset is claimed;
the datasheet does not supply them. This assumption requires physical calibration.

The sole malfunction is **R3, Q1 collector resistor from VC,5.6kΩ→10kΩ**.
It changes Q1 collector/Q2 base and Q2 collector bias (about2.81→4.79V), headroom,
feedback and curve. All other components, input and Level stay identical.
There is no invented gate, dropout or transformer fault. Raw level changes
remain audible; separate RMS-matched files declare their offline gains.

Reject a transformer-only waveshaper: it discards the requested transistor/bias
mechanism. Reject a general per-sample BJT Newton solver for this increment:
preparation is allowed to be expensive, while the callback is fixed-capacity
and bounded. The reduction trades large-signal dynamic accuracy for a small
playable experiment; reduced-reference agreement cannot establish pedal fidelity.
Next: founder listen to low/high healthy versus R3-drift, then measure actual
M7 cycles and physical magnetic/device behavior before considering admission.
