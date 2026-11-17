# ADR-023 — Wallace host serial/parallel comparison

Status: experimental listening study authorized by supplied SF023 assignment;
no production routing or correspondence decision.

Use the exact main Muff, Rotary and Tape and exact unmodified PR77 Hall. Fixed48k,
64-frame blocks. Common Muff Sustain.65/Tone.5/Volume.65, healthy; source calibrated
to0.2 software peak. Tape1.2s medium/.3s delay/.32 feedback/2.4 drive, healthy;
Hall healthy, decay1. Fixed healthy rotary speed command.7. No algorithm changes.

A: Muff -> Rotary mix.35 -> Tape mix.25 -> Hall mix.20.
B: same Muff -> one core plus three actual wet-only processors, sends1, return
weights core.39/Rotary.21/Tape.20/Hall.20. These weights match the expanded serial
control's zero-latency direct coefficient and branch coefficients (.65*.75*.8,
.35*.75*.8,.25*.8,.2), rather than winning by simply doubling the dry signal.
Effects are nonlinear/stateful, so this is a fairness convention, not an equivalence
proof. Optional pre-Muff clean blend independently adjustable, default0; delayed48
frames for nominal Muff FIR latency. Native circuit phase is not perfectly alignable.

B and C have identical sends/returns/core/carving. Return-only Butterworth HPFs:
Tape120Hz, Hall180Hz. No extra complementary EQ. C alone ducks Tape+Hall with a
pre-Muff input envelope,10ms attack/250ms release, low/high.012/.05 normalized
software units, maximum4dB (configurable0–6dB). Exact amplitude interpolation from
unity to10^(-depth/20), no audio pow/log. Rotary stays independent and unducked.
No mastering compressor/limiter or additive sound. One common output gain1.

C-no-carving bypasses only the two return HPFs. C-no-duck is exactly B; validate
this through actual graph execution. Keep weak/strong/zero reset numerical tests
focused on new routing/ducker, not new transistor or reverb campaigns.

Algorithmic delays stay intentional: Tape300ms, Hall early/tail, rotary propagation.
All branches share Muff's latency; do not align echo/room arrivals to dry and erase
the effect. Complete64-frame blocks add no extra queued block to this host renderer.
No physical converter/audio latency claim. Export actual branch/envelope diagnostics.

Use one raw common-gain set plus linear-gain LUFS-matched A/B/C copies. Target-23
LUFS or a common lower target required by-1dBTP headroom; report quantized result.
No claim of improvement until founder listens. Measure actual complete graph and
routing-only cost separately; full Daisy graph admission remains out of scope.
