# ADR-007 — two-rotor acoustics and worn horn-belt traction

Status: **experimental sound/model proposal**, issue #45, 2026-11-12. No founder
musical approval, machine calibration or Moon affliction policy. Gate0 precedes
native code; stop at finished standalone audio under its Engine admission gate.

## Selected reduced model

Borrow the pinned MIT rotary's two-band/directivity/Doppler architecture, adapt
to fixed mono memory and independently driven assemblies. Replace its velocity
LFO/ramp with an explicit **reduced electromechanical** transmission, not a
measured Leslie motor reconstruction. No generic mechanics framework.

Each assembly has motor/rotor angular velocity (rad/s), angle (rad), inertias
(kg m²), bounded proportional drive torque and opposing bearing load (N m):

`Tm=clamp(K*(command-wm),-Tmax,Tmax)`;
`L=b*wr+C*clamp(wr/.5,-1,1)`.

At stick, `a=(Tm-L)/(Jm+Jr)`; required belt torque `Tb=Jr*a+L`.
Release if |Tb| exceeds static capacity. At slip, `Tb=sign(wm-wr)*.7*Ts`, with
incipient direction from required torque when relative speed is zero;
`am=(Tm-Tb)/Jm`, `ar=(Tb-L)/Jr`. Recapture on relative-velocity crossing when
static torque can sustain lock; use momentum-weighted common velocity. This loses
kinetic energy, never creates it. Rotor angles are never reset on release/capture.

Healthy horn Ts=.22 N m; bass .35 N m. The **only fault condition** is localized
horn-belt traction wear: `Ts=.22*(1-.96*max(cos(phiBelt),0)^4)`. Healthy wear=0.
The same kinetic/static ratio, inertia, load and motor remain. Belt material phase
advects at `wm/4` rad/s: one belt lap per four equivalent pulley turns. It is a
spatial worn section passing the contact, **not an independent time LFO**. Motor
motion while the rotor lags carries the worn patch away, restoring traction and
allowing catch-up. Bass has its own unchanged healthy assembly throughout.

The damaged contact is specifically the **belt-to-horn-output pulley**. The motor
pulley is assumed to drive the massless belt without slip; belt material speed is
therefore motor-side speed, even when the output pulley lags. The belt's reflected
mass is included in Jm. This assumption is essential to the chosen material-phase
advection; it would be wrong to infer belt speed from the motor if slip instead
occurred at the motor pulley. Contact-wrap averaging is the smooth spatial lobe,
not a capstan/tension/material simulation.

Horn: Jm=.0002/Jr=.002, K=.12 N m s/rad, Tmax=.14 N m, b=.001 N m s/rad,
C=.02 N m. Bass: .001/.015, K=.12, Tmax=.22, b=.002, C=.02. These are explicit
experimental equivalents, **not measured component/BOM values**. Start at the
healthy slow equilibrium, belt phase pi (unworn section), zero audio histories.
Slow/fast commands .8/6.5 Hz horn, .6/5 Hz bass. Motor error under bearing load
means actual steady speed is slightly below command; report actual speed.

Velocity uses fixed per-sample Euler balance, phase trapezoidal integration; stable
at the declared 32–96 kHz and checked against an independent finer RK4 reference
with exact trig. Quantify convergence, event timing, torque bounds and recapture
momentum/energy. No variable catch-up loop or noisy modulation. A prepared
257-point sine table limits callback math, with interpolation error measured.

## Acoustic approximation and proof

800 Hz complementary first-order split. Horn/drum radii .18/.091 m, 1 m mono
receiver, sound speed343 m/s. Far-field path delay `(1-r*cos(theta))/343` connects
angle/velocity directly to Doppler; no arbitrarily chosen pitch LFO. Source-derived
angle directivity is normalized to <=1, with a causal angle-dependent horn
low-pass as a reduced coloration. Not measured cabinet/horn polar responses.
Linear fractional reads are convex/bounded; quantify their known HF attenuation,
sidebands and high-rate alias limitations. No drive/oversampling/reflection/stereo
or room. Two histories stay private; fixed wet scaling and disclosed RMS matching.

Reference step command is slow before1 s, fast1–5 s, then slow, identically applied
to both states. Deliver actual same-guitar raw/matched audio, synthetic transient/
steady probe, torque/relative motion/phase/velocity/slip/catch trace and numerical
step fixture. Healthy must remain coupled, damaged horn repeatedly release and
recapture, bass motion sample-exact unchanged. Validate reset, malformed prepare,
rates, extremes, long silence/sustain, command transitions, phase continuity,
interpolation, bounded delay/output and callback guards. Founder judges sound.

Unmodeled: exact motor electrical circuit/control, pulley ratios/elastic belt
tension/material/temperature, backlash/wear progression, calibrated torques,
cabinet reflections, exact horn/drum/microphone geometry and directivity, speaker
nonlinearity, stereophony and physical noise. Reference numerical agreement does
not establish physical Leslie fidelity. Canonical and production DSP stay intact.
