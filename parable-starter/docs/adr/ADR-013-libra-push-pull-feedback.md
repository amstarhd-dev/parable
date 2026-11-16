# ADR-013 — Reduced complementary class-AB with closed feedback

Status: experimental, founder sound approval and hardware admission pending.
Issue57 authorizes Option B; no correspondence or Engine change.
[Pre-code decision](../SF013_GATE0.md), [actual source pin](../../third_party/libra/provenance.json).

Use ADI lab14's labelled R_B1..R_B4 resistive-bias drawing plus its documented
2.2Ω emitter resistance per branch. Our adaptation uses9V about4.5V, R_B1/R_B4=27k,
R_B2/R_B3=4.7k, complementary nominal NPN/PNP,1k stage load. These numerical bias
values/load and the guitar gain stage are our design, not stated ADI BOM values.
One physical fault only: R_B1 increases27k→270k. The ADI reference explicitly
warns about thermal instability of resistive bias. This fixed-temperature software
reduction does not establish a safe physical amplifier design or thermal behavior.

Choose constant V_BE0.64V and beta100 forward-active branches. Finite base current,
actual divider loading and emitter resistance give the two current laws:

`iN=max((aN*u+bN-y-0.64)/(2.2+RthN/101),0)`

`iP=max((y-aP*u+bP-0.64)/(2.2+RthP/101),0)`

Here a/b/Rth are the actual resistor-network Thevenin equivalents and the load
obeys`iN-iP=y/RloadEffective`. Thus four conduction regions follow component
conditions; a damaged notch is not painted onto the audio. V_BE curvature, Early
voltage, junction charge, reverse/saturation chemistry, temperature and device
mismatch are omitted. Driver headroom leaves the output devices forward-active.

The own gain stage is an ideal stiff voltage driver, A40,500Hz dominant pole,
±3.5V swing. A30k/10k divider senses **combined output y**: error=`preamp-0.25*y`.
Its40k shunt load remains in feedback-off diagnostics. Backward Euler closes
`uNew-uOld=k*(40*(preamp-0.25*yNew)-uNew)` against each affine region; the selected
solution satisfies its original conduction conditions. No one-sample feedback
lag or iterative callback device solver. All slopes are nonnegative; the
unclipped state derivative is`1/(1+k+k*A*beta*m)<1` for the supported resistive
loads. This says nothing about unmodeled reactive loads or real hardware phase margin.

High-Z1M/10n input, buffered pre-gain4..80 and±4V preamp headroom make the stage
usable for guitar. Level and isolated1u/100k output coupling are common to both
modes; reset represents precharged quiescent capacitors. Overdrive primarily comes
from driver swing limits, not a claim of accurately modeled output saturation.
Final auditions use Drive0.1/0.65; the first higher example0.9 was more compressed.

A full device-level Option A was unnecessary for the first bounded attempt. Option
B produced stable real-guitar output quickly, uses under1KiB state, and shows actual
feedback correction of the resistor fault. No pivot to Bluesbreaker was indicated
by compute/stability checks; whether the voice is convincing remains the founder's
listening decision. Existing main DSP libraries/borrowed facilities were audited;
no unmerged or proprietary model was imported and no shared framework was added.

The1x/2x/4x high-note comparison selects4x97tap; no increase is justified by the
observed≈−52dBr residual. Next: founder low/high A/B, then target-cycle/reference-
amp evidence if retained. No valve/speaker/RevivalDRIVE recreation or hardware build.
