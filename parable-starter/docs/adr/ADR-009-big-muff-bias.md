# ADR-009 — V3 four-transistor fuzz / second clipping-stage bias experiment

Status: **experimental sound/circuit proposal**, issue#49. Founder listening,
physical calibration and pedal admission remain open. [Gate0](../SF009_GATE0.md)
precedes native edits. No topology or correspondence rule approved for production.

## One documented schematic

Use the pinned ElectroSmash American V3 drawing in Gate0, all its signal-path
values unchanged. Q4 input:39k/1u input,470k BC feedback||470pF,47k base return,
10k collector,100ohm emitter. Q3 first clip and Q2 second:10k input/100n coupling,
470k BC feedback||470pF,100k base return,10k collector,150ohm emitter,1uF series
blocking cap into antiparallel1N914-equivalent pair. Sustain100k linear,1k bottom
safety resistor,1uF preceding coupling. No arbitrary pre-gain/static tanh curve.

Tone:39k/10n lowpass and4n/22k highpass,100k linear blend,100n into Q1. Q1 recovery:
430k supply->base/100k base return,15k collector/3.3k emitter;100n output into100k
Volume and explicit1M load. External source1k, ideal9V supply. This selects drawing
facts over inconsistent prose; no mixed circuit editions. No image/layout copied.

## Minimum coherent device/dynamic model

MIT ACME bjt reduction: NPN Ebers–Moll, transport IS1e-14, BF300/BR4; independent
ngspice built-in BJT model with matching parameters. Terminal Is values in ACME
would be transport Is/alpha; do not silently substitute its default convention.
Forward/reverse junctions and finite base current preserve cutoff/saturation,
emitter degeneration and true operating points. Nominal thermal voltage at300K,
not a measured BC239 sample. Diodes IS2.52nA/n1.752, source-compatible generic
silicon assumption. Omit Early/high injection, junction/depletion/diffusion storage,
terminal resistances, noise/thermal/battery dynamics; retain all13 external caps.

Trapezoidal capacitor companions; exact linear elimination of series resistors
and loaded pots (including endpoints). Eliminate the17-node passive network on
stopped control, leaving10 nonlinear port voltages. Prepare the constant matrices;
recompute true nonlinear currents/Jacobians each bounded Newton iteration. A diode
pair's zero-bias conductance may be added to the linear matrix and subtracted
from its nonlinear current **exactly** for invertibility; this is algebra, not an
added physical leak. No general circuit loader/framework, static bias offsets,
block approximation or speculative math cache. Fourfold FIR interpolation and
decimation surround this one implementation;1x fine-grid mode only for science.

Control-only preparation/reset; finite bounded callback, fixed capacities,
explicit residual/failure/containment counters. Healthy and fault share all code,
controls and output scaling. Failed Newton is never presented as fault clipping.
Reference device/DC/all-stage/transient/diode/tone/control and convergence proof,
44.1/48/96k guarded normal/stress, new DI raw/matched and separate history probe.

## One changed bias path / honest outcome

Change only Q2 R15 collector-base470k ->2.2M. The same diode/cap network/controls
remain. This is a wrong/high-resistance/partial-open analogue, not a calibrated
damaged resistor or severity mapping. Candidate1M changes DC but scarcely changes
probe history.2.2M approaches cutoff; pre-code SPICE shows weak-response memory
but overload initially **enhances** the following probe. Do not call that verified
post-overload suppression. Retain both candidate results and the failed musical
hypothesis; ship at most healthy +selected high-R15 experiment. No third candidate,
random gate, envelope-driven dropout or altered diode network.

Founder decides if the actual sound is useful. If the intended congested/choking
recovery is absent, report that criterion unfulfilled rather than normalize
near-silence or fabricate it. A separate future ticket may investigate a different
physical failure with its own science; this does not authorize one now.

## Resource / authority boundary

Fixed16KiB state/text ceilings; exact measured M7 object and stack/layout/calls,
full-processor host timing and bounded-work estimate. No measured target deadline
or calibrated commercial EHX/Seed3 compatibility. No canonical/native Engine
registration or pool refactor. Existing seven firmware links and all original
DSP/journal/living output contracts remain. New source recording replaces only
this demo; prior audition archives and fixtures remain intact.
