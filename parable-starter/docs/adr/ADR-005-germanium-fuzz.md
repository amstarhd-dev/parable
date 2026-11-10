# ADR-005 — SF-003 positive-ground germanium specimen

Status: **PROPOSED laboratory experiment**, independent review/founder listening
pending. Research decision precedes native implementation. Base is updated main
`d628ffc3e0beeca40ca06506c4e1344781be4667` after PR #17 merged. Clean baseline
Debug/Release 13/13 and pinned ARM build passed. No missing required files.

## Sources and reuse choice

| Inspected source | Decision / legal basis |
| --- | --- |
| [ElectroSmash original circuit analysis](https://electrosmash.mas-effects.com/fuzz-face), [Germanium Fuzz](https://electrosmash.mas-effects.com/germanium-fuzz) | Inspected original positive-ground PNP drawing and analysis. Select classic 33k/100k/8.2k/470, direct interstage connection, Fuzz/Volume network. Modern trimmer/Miller-cap variant is distinct. Reference only; do not import illustrations or presume code rights. |
| [Tayda component/polarity notes](https://www.taydakits.com/instructions/fuzz-face/pages/designators-and-components--26) | Corroborates PNP Ge versus PNP/NPN silicon polarity, low input loading and bias sensitivity. Modern build has trimmer/22u variant. Reference only, no diagrams copied. |
| [Holmes, Holters, van Walstijn, DAFx2017](https://www.dafx17.eca.ed.ac.uk/papers/DAFx17_paper_28.pdf) | Primary measured AC128/OC44 Gummel/common-emitter data, Table 1 parameters and equations 1,4–7. Select measured AC128 internal Gummel–Poon plus constant 100pF base-collector capacitance. The paper shows ideal Ebers–Moll omits relevant low-current leakage and high-frequency behavior. Facts/equations cited, no audio/graphics/listing imported; no blanket article code license assumed. |
| [ACME.jl](https://github.com/HSU-ANT/ACME.jl/blob/3832ccafb952ee2ca0a27286c091d613062c23fd/src/elements.jl), `bjt` | **MIT Expat**, exact notice already audited for CMOS. Narrow adaptation of internal GP currents/Jacobian, no Julia/general solver/dependencies. Use measured 25.5mV thermal voltage instead of its fixed25mV, explicit emission coefficients and paper-consistent transport Is. Independently check analytic derivatives; the audited upstream leakage derivative uses the transport rather than leakage emission coefficient, which must be corrected. |
| [Greg Whitehead germaniumbjts](https://github.com/grwhitehead/germaniumbjts/tree/7838a40f939eaee39b37afef1dac031ef45c276a) | **MIT**, copyright2020 Greg Whitehead. Parameter transcription corroborates Table1; retain notice for its adaptation. Its circuit is the paper's modified 36k/62k/11k/680/22u/100n variant, not our classic nominal network. Its VT/CCB/CEB fields are not assumed portable ngspice BJT parameters: implement explicit thermal voltage and constant caps in independent behavioral stamps. No audio/plots imported. |
| [RT-WDF native models](https://github.com/RT-WDF/rt-wdf_lib/tree/4d250f1fc1ad4adaf09d82ce353443174ce2ddd7) | Source header says GPL-2.0-or-later OR AGPL-3.0; audit only. Generic NPN Ebers–Moll constants differ from measured Ge; Armadillo/framework adds cost. No adaptation. Renderer330645ff… has no Fuzz Face reference circuit; no rights inferred from absent root LICENSE. |
| [Overton2006 thesis, official Georgia Tech copy](https://repository.gatech.edu/server/api/core/bitstreams/51260c5f-4066-4229-8a92-ab83c7e615a3/content) | Original ElectroSmash URL failed; official thesis located for measurement/model assumptions. Earlier simplified equations/static lookup and neglected capacitors are not adopted. Reference only; no MATLAB/audio/graphics import. Does not authenticate all AC128/NKT275 parts. |

These sources justify borrowing GP device physics and measured parameters. The
remaining local gap is a dedicated fixed four-node passive/transistor solve with
actual dynamic loading, configuration/state validation and independent fixtures.
A fixed transfer curve/static LUT would lose the feedback, input loading and
capacitor state required by this issue. A full WDF/MNA framework or generic BJT
primitive is unnecessary for one circuit. Generic distortion code is not a fallback.

## Selected exact network and conventions

Battery positive/audio ground **0 V**, negative supply **-9 V**. Q1 emitter at0;
Q1 base receives source through explicit Thevenin Rs and2.2uF; Q1 collector goes
through33k to-9 and directly drives Q2 base (**no interstage capacitor**). Q2
emitter returns through1k Fuzz pot to0; its wiper is bypassed by20uF to0. Feedback
100k joins Q2 emitter to Q1 base. Q2 collector ->8.2k ->output tap ->470 ->-9;
output tap ->10nF ->500k Volume divider, declared1M external load. Both controls
are electrical fractions, not an authenticated mechanical audio taper. Rs=1k
nominal; explicitly vary source resistance/level, not a universal pickup model.

Solve internal coordinates x=-V with a+9 magnitude supply; that is a sign change
of the **PNP positive-ground circuit**, not substitution of silicon/NPN devices.
Published voltages/currents are converted back to physical signs. DC follows from
the transistor/feedback network and never an imposed -4.5V offset.

Both stages use the **same published AC128 fitted sample**, not an authenticated
matched pair or universal AC128/NKT275 constants. Internal GP includes forward/
reverse transport, finite gain, low-current BE/BC leakage, Early dependence and
high-injection gain rolloff; add published constant100pF BC caps. Omit terminal
Rb/Re/Rc, nonlinear depletion/diffusion charge storage, breakdown, noise, battery
impedance and temperature evolution. Thermal voltage is fixed25.5mV; leakage
variation is not a calibrated temperature simulation. Parameter measurements were
finite-range and include reported thermal uncertainty; physical fidelity outside
them is unproven. Nominal 9V can exceed the paper's +/-5V device measurement range.

Before coding, independent physical-sign GP DC research gives approximately Q1
base/collector -0.0732/-0.2115V; Q2 emitter/collector -0.1618/-7.5523V. These
untrimmed fitted-device results differ from the popular collector target -4.5V;
**report that difference** rather than forcing an ideal bias. Bias is independent
of Fuzz/Volume in DC because coupling/bypass caps are open. AC pot loading remains.

## Exactly two degraded conditions

1. `q1_leaky`: Q1 BC leakage saturation current7.546uA ->75.46uA. Controlled
   device degradation/variation through its existing diode current law, not a
   universal failed-part threshold or a temperature claim. Other parameters stay.
2. `feedback_wrong`: feedback100k ->10k, a10x wrong-part/parallel leakage path.
   Input impedance, gain and bias change through the same branch. No invented gate,
   glitch or independent shape. Retain raw levels if either attenuates/mutes.

## Evidence gates

Independent pinned ngspice DC/transient solves of literal paper equations and the
exact passive circuit, including input-source resistance and constant BC caps.
Separate device-current/derivative checks from full-network waveform/bias checks.
Host state/bounds/allocation/nonfinite/reset/overload/recovery/alias/input-loading
and both-fault tests. Ordinary renders/references must report zero numerical
fallback/clipping; arbitrary stress containment is not physical validation.
Same native kernel supplies matched **and common-gain raw-relative** synthetic
WAVs and optional bounded owned PCM16. No third-party recordings.

Use stopped preparation, fixed arrays, bounded Newton work, fourfold oversampling
and explicit failure counters. Measure actual ARM object/state and static work,
not target CPU; optimize only with preserved independent physics if too costly.
Preserve all RAT/CMOS/Soundforge regressions. Registry separates missing hardware,
real guitar, musical approval and integration. Log observed clocks/rework/reuse,
including deliberate copied idioms. Open one unmerged PR, no astrology/Engine/UI.

## Observed implementation refinements (decision still experimental)

192kHz scientific1x comparison matches the standard48kHz4x circuit integration
rate. Against <=.25us independent steps, all-node RMS/peak are1.366mV/62.9mV;
96kHz grid errors are8.70mV/390mV, exposing discretization error rather than
loosening gates. Output-jack error is72.2uV RMS/1.51mV peak. This checks the reduced
equations, not a new physical unit. The independent wrapper's26.85C setting does
not alter the behavioral model's explicit25.5mV thermal voltage.

Initial symmetric diode-step damping unnecessarily limited reverse collector
swing; forward-conduction growth limiting fixes that solver error.24Newton updates
plus one final evaluation remain the per-internal-sample bound. A deliberately
coarse1x48kHz/8.8kHz/.05V diagnostic exits the guarded reverse-junction domain:
29fallbacks/20480samples. Increasing iterations did not fix it; it is rejected as
audio/alias evidence. The same4x case passes with zero fallback/clipping. Accepted
alias comparison uses.01V and reports one folded-third line, not total aliasing.

Measured specimen input impedance at~1kHz is about1048-j22Ohm, appreciably lower
than commonly quoted5..8kOhm examples. Neither impedance nor-7.55V bias is forced
to a target. Both cloned fitted devices, omitted terminal/charge effects and
extrapolated fault parameters limit physical generalization. An owned physical
comparison remains a separate gate. No transistor primitive is promoted for reuse.
