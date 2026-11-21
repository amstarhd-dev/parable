# ADR-006 — physical germanium at a virtual-circuit boundary

Status: **PROPOSED research conclusion; no hardware build authorized**.
SF-004 issue22,2026-11-11. Base main440a992 after PR19. PR21 remains open;
its b2b1bf4 research is context, not a dependency or merged decision.

## Question

Can a physical PNP transistor/pair replace the nonlinear current/Jacobian
calculations while the surrounding circuit remains digitally simulated?
Preserve correspondence rules, the existing DSP/default and all model tests.

## Options and recommendation

**A: queried transistor multiport — no-go as a drop-in replacement.** Four
independent node-voltage drives and four current observations are needed for
the physically connected pair; actual voltage readback adds four measurement
channels. The device supplies currents, not analytic derivatives. Central
differences require five parallel-pair states per coupled evaluation, about
3.766M states/s nominal with a266ns average budget, before digital solve work.
The full static work bound is much higher. Fast external converters exist;
their complete settling, readback, precision and instrument burden still do
not satisfy the unchanged software contract. The100pA residual criterion and
the numerical search domain are especially unsuitable as real-device limits.

**B: continuous nonlinear island — conditional research, not a product choice.**
Analog feedback can solve the coupled relationships without digital Newton
probes or measured Jacobians. It must also contain an instantaneous linear
network/termination around the transistors. An analog companion-conductance
solver could retain digital capacitor histories, but requires programmable
conductances and current drivers. A literal Fuzz-endpoint Norton realization
has opposing1.24A bias currents; it is not a two-transistor accelerator.
Bias centering/current scaling could reduce that burden at additional complexity.

The smaller practical candidate keeps the pair, local bias/feedback, Fuzz/bypass,
BC parasitics and collector loading physically continuous. Keeping input/output
coupling and output termination local makes it more codec-compatible, while
moving most variable passives out of software. Remaining virtual impedances
require real voltage/current coupling and a delay-stability argument. An ordinary
fixed analog fuzz inside an audio send/return is only a comparison baseline;
it does not establish component-in-virtual-circuit substitution.

**Retain all-digital GP as current architecture.** Target CPU deadlines have
not been measured; operation counts alone do not justify hardware. PR21 has
mixed O2 and positive O3 host evidence, not universal host failure. Neither
option is authorized for pilot implementation, purchase, rewiring or PCB work.

## Evidence and tradeoffs

[Quantitative report](../PHYSICAL_BJT_FEASIBILITY.md) includes grounded partitions,
node/current maps, precision/latency budgets, converter references, complexity,
causal failure constraints and a five-question acceptance disposition.

Seven continuous-interface GP/SPICE cases reproduce the original fixtures.
A quantized delayed resistor-surrogate experiment demonstrates how an electrical
boundary can change behaviour or reach converter limits. It is not the GP circuit.
The sampled nonlinear SPICE attempts abort at delivery-event integration; that
is a disclosed numerical blocker, not physical instability or a hybrid pass.
No threshold was relaxed. A validated clocked nonlinear co-simulation and physical
measurements remain missing.

Physical parts offer genuine unit/temperature/charge behaviour, but lose exact
parameter/fault reproducibility, arbitrary virtual passives, and inexpensive
independent processor instances. Physical pickup loading cannot be recovered
retroactively after a high-impedance guitar ADC. Wave-variable coupling merits
analysis; passivity of a linear environment is not proof for this battery-powered
active transistor network.

## Next gate

First validate a clocked nonlinear co-simulation connector on known RC and
GP trajectories. If separately authorized and an existing fixture can be
borrowed, the smallest useful physical test is one source-impedance boundary
around an already wired two-transistor island: measure voltage/current, step
response and loop behaviour with controlled return delay. No purchasing or
construction follows from this ADR. Define current limits and measurement
uncertainty before connecting it; preserve the original all-digital baseline.
