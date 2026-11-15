# SF-017 — Scorpio transistor/transformer listening experiment

Experimental standalone9V Skywave-derived family, not a certified Broadcast clone.
**Listen:** [ten downloadable files and recipe](../samples/scorpio/README.md).
[Decision/omissions](adr/ADR-017-scorpio-bias-and-magnetics.md),
[pre-edit audit](SF017_GATE0.md), [source/license](../third_party/scorpio/provenance.json).
Founder audition/approval is pending; subjective sound quality is not established.

The healthy voice moves from a lower-level, less compressed response to dense drive
as actual feedback resistance increases. On the common take03, Gain0→1 raises raw
RMS0.01791→0.06440 and reduces crest factor14.7→9.0dB. R3 alone drifts5.6k→10kΩ:
Q1 collector/Q2 base6.284→4.583V, Q2 collector2.810→4.793V. Raw fault RMS falls
to0.01266/0.05089; changed headroom/curve and more pronounced peaks persist after
matching. These are measured waveform differences, not a substitute for listening.
Same0.03V source peak/Low Cut0.35/Level0.8; raw and explicitly RMS-matched files.
Credit: Guitar-TECHS Player3 take03, CC-BY4.0; notices/receipts accompany audio.

**Time:** first actual native healthy WAV19min30.857s after starting; prototype
producing code`9521ad1` used193 taps. Final97-tap files use`07a228c` and broader
Gain0/1 examples. No claimed human “usable sound” approval. First-result timestamp
is retained, not reset to exclude research.

**Electrical evidence:** independent ngspice44.2 built-in BJTs versus the explicitly
reduced network: two DC cases,36 static points, six AC points and six transient
samples. Peak errors: DC51µV (gate5mV), static7.31mV (20mV), AC magnitude0.35%
(5%), jack transient11.8µV (5%+1mV). The40Hz linear/nonlinear magnetic comparison
has0.00727 normalized RMS difference and positive stored energy/loading. High-note
7.969kHz nonharmonic spur improves−26.2→−72.0dBr healthy,−14.2→−68.4dBr drift.
These compact checks establish the reduction, not measured device/transformer fit.

**Cost:** full native repeat-pass host cost0.527–0.558µs/frame (GNU14.2.0 Release,
48kHz, four states/presets,432000 frames each; not target timing or WCET).
Pinned ARM14.2.1 M7 object text7968B, data/BSS0; instance9224B. Static callback
frames40+72+24=136B before caller/interrupt margin; stopped prepare has10920B
own frame plus callees. Four internal steps:194 FIR products,140 state MACs,
≤8 bounded10-step searches/Hermite primitives, four ADAA divisions,12 scalar cubic
updates and C10/flux arithmetic. No callback BJT solve, exponential, soft-double,
heap, I/O or locks. **~2000 M7cycles/frame remains an optimistic design budget**;
no established cycle bound, firmware link, target execution or deadline claim.

**Verification:** focused new circuit/CLI tests pass (289 native checks); ASan+UBSan
both pass. One final complete Debug/Release`ctest -j2` is **running**; final results
and exact commands are in[data/sf017](data/sf017/README.md).427 protected previous
implementation files remain byte-identical, including DSP/firmware and fixtures.
Changes are limited to standalone core/lab/tests, independent fixtures, provenance,
ADR/status/registry, listening files and corresponding CI steps. No open PR imported.

**Unproven/next smallest test:** founder A/B approval, physical TY-141P magnetic
parameters/hysteresis and actual transistor spread, large-signal charge dynamics,
changing output impedance, and real Daisy cycles/audio. Only stopped preparation;
no Engine/patch/correspondence or hardware changes. Next: founder listen, then a
bounded target cycle-counter test before any admission decision.
