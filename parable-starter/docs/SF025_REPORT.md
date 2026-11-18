# SF025 — healthy listening milestone (work in progress)

[Healthy WAVs/ZIP](../samples/tiled_room/README.md) delivered before fault work.
Latest revised assignment is [archived](SF025_ASSIGNMENT.md); earlier treated
studio-room/flutter concept superseded. [Gate0](SF025_GATE0.md),
[ADR025](adr/ADR-025-virgo-tiled-room.md) record source research and decision.
Current main767a3c4, independent branch, no other PR modified/merged.

Before edits5/5 focused current-main tests pass,1.06s. Three-rate native healthy
port versus exact pinned upstream with modulation depth0: maximum sample error0
at44100/48000/96000. Native source geometry, delay interpolation, diffuser/tank/tap
signal flow retained; static size.30,4ms predelay,10k input/6.5k tank filtering,
roughly.62s target. Measured48k impulse T20-extrapolated decay.5867s; first stored
reflection5.35ms; high dense early onset is for listening, not a quality verdict.

48k native state336B +48532B caller float storage =48868B persistent; first host
render .0678µs/frame incl output capture stores, GNU14.2/O3/x86_64, not MCU cycles.
All44100/48000/96000 supported; max96k buffer96916B. No callback allocation in
source; guard tests, sanitizer and ARM proof pending. No production integration.

Healthy mono/stereo/raw/wet, impulse/transient and exact source/credit available.
Main.65dry/.35wet, raw gain1; constant-gain−23LUFS copies/receipts. Distinct PCM70
flat/running-stopped decay, original early-reflection controls and transfer details
are not copied or verified. Located public manufacturer/review demos; no controlled
same-input physical PCM70 comparison, copied demos or authenticity claim.

Remaining work: one defensible same-network fault after healthy delivery,
focused tests/resources/ARM evidence, prior-room comparison and final report.
PR intentionally draft/unmerged while this is incomplete; no acceptance success
or founder musical approval claimed.
