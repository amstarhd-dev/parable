# SF-013 — Libra: reduced push–pull feedback overdrive

Experimental Option B, not a commercial amp clone or measured2N3904/2N3906 model.
[Nine WAVs](../samples/libra/README.md) · [ADR013](adr/ADR-013-libra-push-pull-feedback.md) ·
[source/rights](../third_party/libra/provenance.json) · [pre-code decision](SF013_GATE0.md).
First native guitar WAV: **17min32.967s** from task start. Founder hearing/approval
is pending; actual files, not a sinusoidal transfer curve alone, are the deliverable.

ADI's four-resistor bias topology supplies genuinely complementary branches. The
own A40/500Hz gain stage senses their **combined output** through30k/10k feedback.
Each sample solves that closed loop in at most four affine conduction regions.
One fault only: R_B1 increases27k→270k. Healthy idle current≈0.65mA per branch;
fault upper is off and lower≈2.9µA. Fault DC output≈−2.84mV with feedback versus
−26.1mV without. At explicitly matched nominal0.2V output, the fault/healthy RMS
difference falls0.592→0.222 with feedback—a2.66× suppression, not complete repair.
The diagnostic input amplitudes differ to match nominal healthy output; **listening
A/B input and Level remain identical**. No notch/dropout/filter fault is added.

Final low/high Drive0.1/0.65 gives healthy raw RMS0.0312/0.1618 versus fault
0.0274/0.1586. The higher preset is less compressed than prototype0.9. Disturbance
is most consequential near crossover/low amplitudes; whether it sounds smooth,
persuasive or useful remains a founder decision. Raw and separately matched files
retain the real level effect. Source is approved CC-BY Guitar-TECHS Player3 take03.

**Checks:** focused Debug4/4, Release4/4 (native/CLI and registry), ASan+UBSan2/2.
Independent SPICE **piecewise equivalent**, not a calibrated builtin-transistor
claim: static voltage error0.363µV, current5.22nA, short-transient peak1.481mV.
Complementary conduction, resistive100Ω/1k/10k loading, control endpoints, actual
pole/KCL residuals, reset/silence/invalid values and callback guards pass. At7.969kHz,
1x/2x/4x nonharmonic spur is−25.7/−38.9/−52.3dBr healthy and−23.4/−38.8/−52.7
fault;4x is retained. Results include discretization/filter differences and are not
an alias-free claim. [Exact commands/raw results](data/sf013/README.md).

**Cost:** full native host **0.239–0.275µs/frame**, GNU14.2.0 Release,432000-frame
repeat per mode after builds/tests. Actual pinned M7 object text2872B/data0/BSS0;
instance968B. Four internal steps,≤16 region candidates and194 FIR products/base
frame. No iterative junction solve, table search, runtime libm, heap, lock or I/O.
Error-only memset is audited to32bytes. Known static audio frames120B before
caller/interrupt/helper margin; stopped prepare1136B own frame. **No measured
Daisy cycles, firmware link, boot/audio or stack high-water evidence.**

**Limits/next:** constant V_BE0.64V/beta100, ideal driver, resistive bias, no charge,
thermal or reactive-load model. Overdrive primarily comes from driver swing limits;
this is not accurate output-device saturation chemistry. ADI warns about thermal
stability of physical resistive bias. Option B met compute/stability/guitar-rendering
gates, so no Bluesbreaker pivot was indicated; musical acceptance is still open.
452 prior source/firmware/fixture files and shared utilities stay unchanged. No
complete old suites ran locally; global CI remains intact. No Engine/patch/astrology/
phone/storage changes. Next: founder A/B, then target timing/reference-amp evidence
if retained. The PR is for independent review and must remain unmerged.
