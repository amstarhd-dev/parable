# ADR-020 — experimental Gemini coupled rooms

Status: selected for issue73 experiment; musical/hardware acceptance pending.

Use two four-line, damped Householder FDNs derived from the MIT Signalsmith mixed
feedback pattern. One bidirectional port connects their feedback networks.
A uses11.3/13.7/17.9/19.7ms propagation, nominal uncoupled low-frequency RT60=.65s;
B uses29.3/37.1/43.7/53.1ms and1.9s. Absorption poles5.5k/3.2kHz. These are reduced
propagation times and attenuation, not asserted physical dimensions/RT60 fits.
Delay rounding and coupling change measured composite decay. Early fingerprints
are the first traversals of those same loops; no separate image-source claim.

At each frame read eight delayed waves, damp each, Householder-reflect each room,
then mix the two channel0 returns: A'=(1-c)A+cB; B'=cA+(1-c)B. Fault changes
A'=(1-c)A-cB ONLY. This identifies the B electronic return lead to A as miswired.
Local losses and excitation follow; no zero-delay algebraic feedback. Readout is
a fixed signed sum of each room's four returning waves, mono A+B at fixed gain.
The main input excites A only. Separate A/B inputs and taps are laboratory ports.

Both connection variants are contractions; Householder is orthogonal; per-line
losses<1 and one-pole absorption is nonexpansive. Thus the unforced network is
stable at all permitted apertures. This is a bounded acoustic reduction, not a
validated building simulation or specific commercial room algorithm.

Use fixed caller-owned buffer, sliced at prepare for actual44.1/48/96k rates;
class owns no heap. Stopped preparation only for geometry/rate. Live owner-thread
mix/aperture/polarity requests use20ms ramps; polarity sweeps through zero on that
one coefficient. No cross-thread interface, preset activation or Engine admission.
Alternative whole Signalsmith reverb import offers diffusion/modulation and more
polish but misses explicit two-room partition, brings framework dependencies,
and costs more memory. This reduction exposes causal transfer and stability with
low fixed work, at cost of limited density and uncalibrated acoustic parameters.

Prototype observation: aperture.35 produced only~8% raw guitar difference.
Selected aperture.85 after~37% raw difference with all other parameters identical.
The permitted0..1 range retains the same contraction proof; no second fault.
