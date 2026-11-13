# ADR-011 — Centaur clean-gang wiper contact

Status: **experimental; independent review and founder listening pending**.
Issue53 authorizes one standalone voice, not patch/correspondence activation.
[Gate0](../SF011_GATE0.md) pins the original April2009 S698 trace and pre-code
SPICE probes. Select P1A clean wiper contact0Ω→100kΩ; P1B gain gang remains
healthy. All other parts and controls are identical.10k/1M/1G were research
probes, not extra runtime faults or a severity lane.

Keep actual input buffer, dual-gang inverse tracking, both frequency-shaped clean
routes, shunt germanium clipping, loaded summation, active tone, output/bypass
bleed and passive Vref. This is not a generic dry/wet blend or modified TS808.
Fixed nominal pump-domain rails are explicit assumptions; rail sensitivity and
physical omissions must remain visible. No measured original unit or hardware
claim. Preserve source labels: P1A(clean)/P1B(drive), R241 tone feedback,
R26/R27 output bleed; other drawings number them differently.

Reuse the audited MIT diode/pole foundation and owned fixed-network/capacitor/
FIR/pivot/guard/lab facilities. ChowCentaur is meaningful audit-only prior art;
its independent WDF path/termination assumptions require substantial work for
this single-contact mutual-loading experiment and its BSD-3 source is outside
the unchanged code allowlist. No generic framework or policy expansion.

One specialized bounded coupled solve preserves loading and feedback. Four amp
states plus one diode voltage, stopped exact passive reduction, exact electrical
pot endpoints. First investigate4x with refinement; disclose failed prototypes,
actual candidate counts/host cost and pinned ARM object/call/layout/static work.
Do not infer future2000cycle fit from a successful compile or relax fidelity to
make an unmeasured target look admitted. No production memory changes.

Analyze actual loaded path currents, not independently filtered audio buffers.
The R16 summing current also carries clean-side signal through C11/R15. Group
drive=C10 current, clean1=R19 current, clean2=C11/R15+R17+C12/R18 currents with
explicit signs. Their sum obeys real-node KCL. Analysis taps and finite-amplifier
feedback voltage contributions are diagnostic, not separately playable final
outputs or a naive dry/wet sum. Verify phase/cancellation in SPICE and native.

New licensed guitar source, three Gain conditions, raw and bounded offline-matched
WAVs, controlled dynamics/chord and electrical/alias evidence. Match never hides
near silence, numerical containment or another gain change. No forced musical
verdict. No Engine/canonical/slots/storage/pools/firmware/living integration or
other PR edits. Next gate is founder audition, then separately measured hardware
admission if the sound merits it.
