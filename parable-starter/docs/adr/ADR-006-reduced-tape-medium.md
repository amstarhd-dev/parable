# ADR-006 — cyclic reduced tape medium and weak erase experiment

Status: **experimental proposal for independent review / founder listening**.
2026-11-12; issue #43 approves the voice/fault design target, not an activation
rule or physical-fidelity claim. [Gate 0](../SF006_GATE0.md) precedes code.

## Choice and rejected alternatives

Use one spatial cyclic array of normalized remanent magnetization, three distinct
head positions and fixed transport. Borrow MIT DaisySP circular storage, one-pole
and rational SoftClip primitives, not a generalized delay framework. Ordinary
feedback delay with an extra ghost tap is rejected: it cannot establish an erase
defect. Setting old state to the new sample at the write head is also rejected:
perfect overwrite would destroy the very information a weak erase leaves behind.
GPL CHOWTape is audit-only; full Jiles–Atherton/Preisach magnetics are not imported.

## Explicit reduced physics

Tape passes **erase -> write -> playback -> erase**. With write cell w and N
cells, erase visits (w+E)%N, playback visits (w-D+N)%N; E+D<N. A given material
cell is erased E samples before writing, then played D samples later. Default
loop 1.2 s, D=.3 s, E=.015 s; fixed speed .1905 m/s (7.5 in/s) gives .2286 m
loop, .05715 m write/play spacing, .0028575 m erase/write spacing. Sample rounding
is explicit. No variable-speed interpolation or wow/flutter in this first test.

Erase: **m_e = r m_old**. Healthy r=0 (ideal complete erase), defect r=.85
(85% residual retention / 15% erase efficiency), representing insufficient AC
erase field exposure. It is not a calibrated erase-current conversion or a
spatial/head-alignment claim. Both states have the same finite write fraction
**a=.6**: **m_new = (1-a)m_e + a S(g*x_filtered + f*y_play)**. This is a
phenomenological population/partial-reorientation approximation: the record field
does not fully overwrite the previous remanence. It is justified by magnetic
history and incomplete-overwrite literature, but a single scalar is not a
hysteresis loop, depth-domain simulation or measured tape specimen. The fault
changes r alone; a, filters, drive, heads and feedback remain equal.

S is DaisySP's bounded rational SoftClip; record/replay one-poles represent
bandwidth loss (default 4.5/6 kHz). Record drive is on the external input only;
feedback does not receive that gain. Read playback BEFORE erase/write at each
tick, then advance transport exactly once. No ghost data, recording reference or
extra delay exists inside the processor. The same cells carry old and new
information. Histories initialize/clear to zero with processing stopped.

Because |S|<=1 and the write is convex, |m|<=1. Both one-poles are convex.
With 0<=f<=.8, 0<=r<=.95, a=.6, the maximum history contraction is
**q=(1-a)r+a*f<=.86** (SoftClip slope <=1); zero-drive histories decay rather
than self-oscillate. With feedback zero, old phrase gain after each revolution
is exactly **(1-a)r = .34** in the fault, zero after healthy erase; filters do
not create a 1.2/2 s delayed phrase. Output wet mix is convex, not auto-normalized.

No AC bias oscillator, coercivity distribution, actual erase current/head field,
minor loops, inter-particle fields, depth/track alignment, print-through, noise,
oxide wear, head-gap wavelength response, compression calibration or commercial
tape-machine identity is claimed. Soft saturation/filtering alone is not physical
tape fidelity. Their role here is a minimal recognizable degrading echo, with
the medium-memory experiment distinguished from that coloration.

## Proof and implementation boundary

Independent high-precision scalar material-cell/event reference plus closed-form
multi-turn erasure; feedback-zero A -> different B and B-only counterfactuals.
Healthy must remove A, fault must preserve measurable A during B replay, and its
survival must contract geometrically. Include listening WAVs from the same licensed
clean guitar, raw/matched gains, isolated ghost, spectral/energy/node receipts,
callback guards, full regressions and rate/geometry bounds. Founder judges sound.

External caller-owned float storage is prepared/cleared off audio and must outlive
the stopped processor. No copy/move aliases, callback allocation or cleanup.
Not a canonical patch processor or prepared-bundle member. Engine integration is
deferred under Gate 0's memory/lifetime/hardware gates; no format/version change.
