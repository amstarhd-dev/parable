# ADR014 — one reciprocal Lockhart cell and upper-feed resistor drift

Status: **selected for standalone experimental SF014**, issue58. Founder
listening, independent review and target admission remain open. No production
topology/correspondence, pilot expansion or Engine/firmware registration approved.
[Stage0](../SF014_GATE0.md) pins sources, baseline, circuit, candidates and budgets.

Use the published complementary PNP/NPN Lockhart topology with shared bases and
collectors,15k emitter/feed resistors to±15V,10k collector load and isolated ideal
unity inverting readout. Generic reciprocal Ebers–Moll transport IS1e-17A,
BF100/BR1,T300K; actual three-node loading and base currents remain. Borrow the
audited MIT ACME current/derivative foundation, not unlicensed/GPL folder code.
The reference supply is not Daisy DAC headroom; no physical unit is calibrated.

Choose **one cell**, not a cascade. Stop/prepare solves that fixed nonlinear
network and implicit derivatives. A512-interval cubic table over±3V stores
rounded values/slopes and the exact integrated cubic, reproducing the original
curve within microvolts. Runtime4x/193tap FIR and first-order ADAA suppress aliases
without callback exponentials or iterative solves. Retain accurate direct and
independent SPICE references; do not call the LUT an unrelated polynomial fold.
All limits and rejected1x/2x/4x options remain visible. ADAA's wanted-waveform
change is a tradeoff, not universally better accuracy or proof of inaudibility.

Exactly **upper PNP feed RP15k→22k**. Lower RN, load, rails, devices, input/output
calibration and all filters remain identical. Both fold roots move negative and
raw readout DC rises~.794V; this comes from the actual branch currents/KCL,
not a synthetic offset or one-sided waveshaper.33k moves both folds outside
the chosen guitar drive and is research-only. No severity control or second fault.
Common100n/100k output coupling blocks steady DC. Reset models a precharged
capacitor; always retain raw cell bias/transfer/current evidence separately.

Drive0…6 is a finite same-owner input gain; three audition positions1/3/6 with
the exact Centaur take03 calibrated.2modelV. OutputVolts/8 is common software
headroom. No random artifacts, synthetic gate/sitar envelope, pitch/FM/oscillator,
extra effects, generic solver or silent factory-bank/astrology decision.

Use main's unchanged shared laboratory/guard/SPICE/source-preparation tools and
owned FIR/FFT patterns. Required gates: independent full circuit/DC/current/fold/
load/rail/harmonic and high-rate alias/refinement;44.1/48/96k controls/reset/
silence/finite/guard tests; raw/common/RMS same-take WAVs plus controlled signals
and explicit RAT/Muff/CMOS comparison; full Debug/Release and sanitizers; old
golden preservation; pinned M7 calls/frames/state plus conditional cost estimate.
No successful host/object build certifies target cycles, physical audio or musical
quality. Preserve all open PRs, existing firmware/patch/storage/living behavior.

Physical envelope: ideal input/readout buffer and supplies, nominal static BJT
law, no intrinsic junction/diffusion storage/Early/high-injection/noise/temperature
dynamics or measured components. Memoryless cell plus declared linear coupling
does not establish arbitrary transistor recovery. Future embedded admission and
any proposed voicing adjustment require separate measured/audition decisions.
