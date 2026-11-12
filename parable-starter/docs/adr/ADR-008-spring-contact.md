# ADR-008 — passive modal spring body and displaced support

Status: **experimental sound/physics proposal**, issue#47, 2026-11-12. No musical
approval or astrology activation. [Gate0](../SF008_GATE0.md) precedes native edits.

## Body and source boundary

Narrowly adapt MIT Rings positive-stiffness mode placement and spatial weighting.
Two finite modal spring paths, 96 modes each: base26/31 Hz; harmonic n*base times
stretch, stretch starts1 and increases by stiffness.04, stiffness *=.98 each mode.
This produces dense inharmonic, dispersive ringing. It is **not** the complete
helical spring PDE, longitudinal/transverse/backward-wave branches or measured
tank poles. No generic Schroeder hall is relabeled as a physical spring tank.

Each mode has equivalent mass .0003 kg and frequency w. Store mass-normalized
`x=w*sqrt(m)*q` and `v=sqrt(m)*qdot`; **E=.5*sum(x²+v²)** joules. Source/receiver
weights are cosine spatial projections, source position .03, pickup .91; support
contact at .37 of path0. Force divides equally across paths, full-scale .002 N.
These are explicit equivalent assumptions, not coil/tank measurements.

Fixed2x stepping: first multiply x/v by exp(-sigma*h), sigma=3+8*(f/8000)² /s;
apply a force impulse h*F*shape/sqrt(m) to v; then exact lossless oscillator
rotation. The amplitude-loss operator is a passive material-loss approximation,
**not exact viscous-dashpot integration**. Mechanical velocity in the lossless
substep is qdot. This split has a simple positive energy/decay bound; document
and measure sampling/contact approximation rather than invent full fidelity.

## One support change / passive impact

Only clearance changes: healthy5 mm -> displaced path0 support20 micrometres.
Path1 has no changed support. Contact displacement y=sum(d*x), d=shape/(w*sqrt(m));
point velocity u=sum(c*v), c=shape/sqrt(m), both restricted to path0. If |y|>gap:

1. Project position along d onto the boundary; removes potential energy
   `(y²-gap²)/(2*dot(d,d))`. This is **numerical overshoot correction**, reported
   separately and checked for convergence, not credited as restitution.
2. For outward u, impulse `J=-(1+e)*u/dot(c,c)`, e=.55, and `v+=J*c`.
   Physical kinetic loss `(1-e²)*u²/(2*dot(c,c))>=0`. It redistributes existing
   energy across modes, potentially increasing HF modal energy while total falls.

No new noise/sample, thresholded crash mix, randomization or extra delay. The
same body, drive and pickup operate in both states. Subthreshold state never
contacts; strong excitation must actually move it through the boundary. Truncated
modal point contact/mass and rigid restitution are uncalibrated approximations;
no elastic/plastic Hertz force, gravity, tank shell or mutual coil collisions.

## Energy, tail and audio

Input-port budget **2e-6 J**: before each force kick, solve a scalar quadratic
for a bounded reduction of requested force only if it would exceed the energy
budget. Report limiting; weak/strong/guitar proofs should not rely on it. It is
a common numerical input safety policy, not the malfunction. Rotation is energy
preserving, damping/contact/clear only remove energy. Track the entire balance
including signed input work, material loss, projection loss and restitution loss.

Unforced E(t)<=E0*exp(-6t), including contacts. At6 s the worst-case normalized
return bound is below1.3e-8, then clear passively; no endless tail or audible gate.
Prepared Cauchy–Schwarz pickup scaling and FIR L1 normalization guarantee bounded
wet return. Required controllable wet mix/return; raw gains documented separately
from offline matching. 193-tap2x output low-pass, cutoff .42*base sample rate,
group delay48 base samples. No audio callback coefficient work/JSON/heap/locks/IO.

Independent small spring/contact calculation and passivity/decay bounds, plus
multi-rate convergence, weak/strong/impulse, IR/energy/HF traces and same DI guitar
must establish the mechanism. Founder judges whether it sounds like a convincing
spring crash. Keep full host specimen standalone; separately propose a smaller
embedded approximation after real CPU/state admission and A/B, no pool rewrite.
