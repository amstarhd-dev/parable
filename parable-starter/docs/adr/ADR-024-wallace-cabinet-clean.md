# ADR-024 — Real cabinet measurements and independent clean path

Status: experimental host listening decision, authorized SF024 assignment and
mandatory clean-path clarification; musical/hardware approval remains open.

Reuse unchanged Muff/Rotary/Tape/Hall and SF023 return mixer. Select MIT
FFTConvolver with64sample partitions, portable scalar Ooura backend (disable SSE
for repeatable allocation accounting and portable comparison). Full measured IRs,
resampled44100→48000Hz externally, one scalar per kernel normalizes maximum FFT
magnitude to unity. Retain original timing/phase/tail; no minimum-phase conversion,
synthetic substitute, added EQ or automatic polarity correction.

Primary pre-cab: split before Muff. Independent clean gain .50, Muff .75;
Rotary .18, Tape .12, Hall .12. Clean bypasses all four effects; delay it48frames
only for Muff nominal FIR alignment. Effects receive that same clean-delayed input,
never Muff. Wet-only returns with unchanged Tape120Hz/Hall180Hz highpasses;
ducker disabled. Sum all then shared cabinet. This clean is unprocessed by effects,
though intentionally colored by the common cabinet. No independent dry duplicate
inside Tape/Hall. Clean-disabled **control only**: clean0, other sends/gains same.
Variant7 clean .75/Muff .45. Gain controls independent, default clean is substantial.

Post-cab: (.50 clean48 + .75 Muff) → cabinet, preserve that complete cabinet bus
at unity to final mix, add wet Rotary/Tape/Hall driven from it. Same .18/.12/.12
returns/HPFs. Rotary here is studio placement, not a physical Leslie reconstruction.
Case9 ducks only Tape/Hall, max2.5dB,10ms attack/250ms release, input envelope
thresholds .012/.05. Case10 expansive returns .25/.32/.40; direct bus remains.
Other processor settings exactly SF023. No amp stage D: no credible drop-in.

Convolution has zero additional buffering latency, to be verified against direct
double-precision convolution and delta timing. Recorded IR delay remains; wholemix
shares one cabinet so no parallel cab-delay compensation is needed. Do not align
musical room/rotor/echo arrivals. Muff physical phase cannot be removed by48sample
alignment. Measure actual cancellation/correlation; do not infer quality from it.

Export weighted clean before/aftercab, fuzz and ambient contribution stems.
Separate offline diagnostic convolution passes produce stems, not extra live
convolvers in the timed graph. Verify sums and pre-cab clean-on minus clean-off.
Use matched fullmix WAVs plus shared-gain clean solos; clean-disabled is never a
principal/default topology. Test default/validation, true routing, reset, finite
bounds, callback guards and deterministic recordings. No new general graph.
Host cost/memory and listening evidence do not establish Daisy admission.
