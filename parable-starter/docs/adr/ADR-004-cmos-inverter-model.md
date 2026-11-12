# ADR-004 — CMOS inverter Soundforge specimen (SF-002)

Status: **PROPOSED experiment; independent review and founder audition pending**.
Research gate recorded before native DSP implementation. Start: main
`2cc9412c1968d1c229bcbedba7fb6e19dfd6f416` after PR #15, including its reviewed
license allowlist. Clean host Debug/Release 10/10 and full pinned ARM build passed.
No missing issue-required repository files. No astrology or patch integration.

## SF-005 amendment — experimental Gemini healthy / bias leak

Issue #41 authorizes a narrow playable A/B and conditional native registration.
[SF-005 Gate 0](../SF005_GATE0.md) preceded source edits; [measurements/commands/
limits](../SF005_REPORT.md) record the outcome. **Founder musical approval remains
pending.** This amendment does not approve an astrology/fault rule or a commercial
clone. The original SF-002 research and three laboratory states below remain.

Keep the same MIT-adapted device equations, stiff 9 V reduced network, 4x FIR and
bounded 2x2 trapezoidal/Newton solve. Add one physical leakage conductance from
second gate to VSS. Healthy is open (0 S); damaged is **220 kohm**. A 330 kohm
candidate shifted bias but gated less strongly; 100 kohm nearly muted the source.
The selected value is an inspectable experiment, not a characterized failed IC.

The old 1 Mohm -> 100 kohm feedback resistor changes AC gain, **not DC bias**:
at DC the capacitors open and zero gate current requires gate=output. With the new
shunt, gate2=output2*Gfeedback/(Gfeedback+Gleak); output KCL determines the loaded
operating point. At 9 V, healthy gate/output are 3.482808651 V; damaged gate is
1.622727171 V and output 8.998759769 V. The modeled NMOS is off at rest, so larger
input attacks are needed to enter conduction. Coupling/feedback capacitor histories
are initialized to this split equilibrium; the same branch participates in every
audio solve/Jacobian. No random glitch, post gate or numerical fallback creates
the sound. Poorly calibrated extrapolation outside measured |Vgs|=2.5..6 V remains
an explicit physical limitation, especially in the damaged state.

Register `GeminiCmos`, revision 1, additively in patch-v1: required normalized
`drive`, `volume`, and strict `biasLeak` boolean quantity. The boolean selects
these **fixed** electrical states; no severity, hidden defaults or mid-callback
reconfiguration. Old readers reject the unknown type; old bytes/semantics remain.
Changing the selected component interpretation after acceptance requires a new
revision. This experiment does not ratify missing Parable Lexicon semantics.

Use one fixed tagged RAT-or-CMOS state in Engine, with native control values
sharing their existing 12-byte space. At most one nonlinear declaration total,
including disabled stages; do not silently allocate another effect or enlarge the
four-bundle pool. Stopped control preparation and existing PAR-011 ownership remain.
ARM measurements confirm unchanged Engine 11,040 B / Runtime 45,224 B. Mixed
RAT/CMOS chains require a future measured resource decision, not this increment.

Use the same openly licensed DI recording for both states and publish raw/common
gain **and** matched-level WAVs. RMS matching is offline listening treatment,
never inserted into Engine. Reject normalization of fallback, rail clipping or
near-silent numerical residue. Real-recording provenance is separate from the
unchanged Soundforge DSP-code import allowlist. No wearable or new framework work.

## Research and choice

| Source inspected | Evidence and decision |
| --- | --- |
| [Köper/Holters, DAFx2020, Taming the Red Llama](https://dafx2020.mdw.ac.at/proceedings/papers/DAFx2020_paper_21.pdf), CC-BY-3.0 article | Equations 9/17/18, measured MOSFET fits, Tables 1/3 and Figure 13 support a specific CD4049 model. Published measurements concern their DIY reference unit, not every CD4049 or a commercial clone. Cite factual parameters/equations; no illustrations or recordings imported. |
| [ACME.jl published implementation](https://github.com/HSU-ANT/ACME.jl/blob/3832ccafb952ee2ca0a27286c091d613062c23fd/src/elements.jl), `mosfet`, MIT Expat in LICENSE.md | Selected narrow port: piecewise MOSFET current, polynomial threshold/transconductance and derivatives. Retain exact MIT notice. Do not transplant Julia, its dependencies or a general circuit solver. |
| [Author supplement](https://lkoeper.gitlab.io/dafx-2020-cmos-llama/) | Corroborates exact fit values and reference connections. Its circuit listing has no separately located code license; reference only, not copied code. Paper's nominal R/C values are selected rather than claiming to reproduce its measured component values. |
| [TI CD4049UB/UBE SCHS046L](https://www.ti.com/lit/gpn/CD4049UB), [SCHA004](https://www.ti.com/lit/pdf/SCHA004) | Manufacturer part identity, unbuffered operation, supply limits, output sink/source asymmetry and transfer bounds. CD4069UB is not treated as interchangeable. Documents are reference only; no graphics imported. |
| [TI SCHM019A PSpice model](https://www.ti.com/lit/zip/SCHM019) | Actual archive inspected: generic logic behavioral model with SGN switching/propagation delays, not measured analog transition-region MOSFET behavior. All-rights-reserved notice; not imported, adapted or used as analog truth. |
| [Tonepad project 65](https://www.tonepad.com/project.asp?id=65) | Establishes Anderton/Tube Sound Fuzz lineage and a simplified variant. Download redirected to its warning page; the complete drawing was not retrieved. No layout/drawing copied or assumed permissively licensed. |
| [Obsidian-B7000](https://github.com/tehguitarist/Obsidian-B7000/tree/4b91389065f59d754b3f383d9f94fa011f0d81da), AGPL-3.0 | Inspected Clipper.h/nonlinear-component-modeling.md/LICENSE. A different bass-pedal path with fitted inverter curve/feedback; audit only. No implementation, parameter fit, curve or processing code copied/adapted. |

The scientific model and MIT implementation supply the credible reusable starting
point. The genuine gap is a small fixed-capacity native implementation of this
specific circuit, with independently checked time discretization and resource
evidence. A generic waveshaper, full Julia framework, binary logic model and AGPL
port are rejected for circuit suitability, deployment cost or license scope.

## Exact reduced circuit

Two biased CD4049UBE-style unbuffered inverters, each represented by complementary
MOSFET currents using the published extended fit. Nominal signal-path values from
the paper: input R=100 kOhm/C=68 nF; first feedback (100 kOhm + 0..1 MOhm linear
Drive pot) || 51 pF; interstage 33 nF; second feedback 1 MOhm || 100 pF; output
10 uF into a 10 kOhm Volume pot and declared 1 MOhm external load.

**9 V ideal regulated rail at the IC**, not a 9 V battery through the original
1 kOhm/330 uF power path. The supply dropper, battery impedance, supply dynamics,
protection diode and unused gates are omitted. This is deliberately the reduced
signal-path circuit, not a calibrated full Anderton/Red Llama clone or modeled
supply sag. DC self-bias and finite loaded output behavior must be solved from
the same MOSFET/feedback relationships as the signal; no arbitrary bias knob.

Measured fit parameters cover |Vgs|=2.5..6 V. Outside that range, freeze the fitted
parameter functions at the nearest measured endpoint, keeping actual Vgs in the
MOSFET equation. This explicit unvalidated extrapolation avoids negative/nonpassive
polynomial behavior; it is not manufacturer characterization. Intrinsic gate/
junction capacitance, input/body protection currents, temperature/noise/tolerance
and package power behavior remain unsupported. Ordinary controls are electrical
Drive/Volume positions, not measured mechanical pot tapers or hardware calibration.

Dedicated two-output nonlinear solve with capacitor companion histories; bounded
work, fixed state and coefficients prepared with audio stopped. Whole-circuit
4x oversampling and a 1x comparison mode. This does not create a general MOSFET,
netlist or SPICE runtime. Numerical containment/fallback must be measured and
distinguished from physical behavior; normal listening/reference cases must not
silently rely on it.

## Two component experiments

1. Interstage capacitor 33 nF -> 330 pF (100x wrong part): changes the second
   stage's AC drive/bass/attack through the same coupling network.
2. Second feedback resistor 1 MOhm -> 100 kOhm (leak/parallel-path equivalent):
   changes gain and response through the original feedback branch. This is not
   an arbitrary distortion-shape preset or a floating gate experiment.

No additional fault family. Unknown/invalid configurations reject explicitly,
without changing the working prepared state. Reconfiguration is stopped-only.

## Evidence required before handoff

Independent offline ngspice DC/transient comparison of literal circuit stamps and
published MOSFET equations/parameters, plus stated manufacturer/source bounds;
not expected data emitted by native DSP. Independent solver agreement validates
this reduced model, not the paper's physical-unit match or manufacturer fidelity.
Host safety/state/alias/DC/recovery tests; owned synthetic healthy/dynamic/fault
WAVs from the shared C++ kernel with raw/final levels and exact settings; bounded
PCM16 external input via existing Soundforge I/O. No third-party recordings used.

Register separate evidence stages, retain MIT attribution and precise source
hashes, keep all LabRAT regression bytes unchanged, compile the actual ARM object
and measure instance/text sizes. Log observed elapsed gates, reuse/new/duplicated
LOC, manual steps and rework. No uncontrolled-project speedup or target-CPU claim.
No code import beyond the explicitly licensed MOSFET model and owned internal
laboratory/DSP techniques. Deeper Soundforge abstraction remains deferred.
