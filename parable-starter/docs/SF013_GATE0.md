# SF-013 — bounded Option B feasibility / pre-code decision

Start2026-11-16 00:24:12UTC, current main767a3c402b972fc20df8aea0729adf794f4b7a38.
Issue57 active brief supersedes exhaustive original gates. AGENTS, Constitution,
README/status, ADR001 and Soundforge recipe read; root AGENTS absent, project
AGENTS present. Pre-edit utility/registry baseline is the focused gate. No unmerged
PR imported; Capricorn69 and65/61/63/56 remain independent. No production integration.

Select Option B from ADI lab14's Theory resistive-divider drawing R_B1..R_B4,
combined with its documented2.2Ω per output emitter (two1.1Ω) and complementary
2N3904/2N3906 family. Own9V design about4.5V: upper/lower rails±4.5V,
R_B1=R_B4=27k,R_B2=R_B3=4.7k; stage load1k plus40k feedback divider.
The one fault is R_B1 only27k→270k. Reference labels are visible; chosen numerical
bias values/load are our adaptation, not claimed original ADI values. Fixed300K;
ADI warns that fixed resistive bias is thermally unsuitable without compensation.
No thermal/actual hardware design claim follows.

Forward-active approximation: V_BE=0.64V, beta100, finite base loading and emitter
resistors retained. Each branch conducts only when its own base-emitter condition
allows it. Four affine conduction regions follow Kirchhoff's law; no imposed notch.
An explicit own voltage-gain stage A40,pole500Hz,driver rails±3.5V senses
`preampInput - betaFeedback*combinedOutput`. Rf30k/Rg10k gives betaFeedback0.25;
the40k load remains when feedback is disabled for diagnostics. Stable implicit
backward-Euler closure can enumerate four regions exactly each internal sample.
This avoids a large transistor Newton solve, while retaining real opposing
conduction/feedback. Full Ebers-Moll main models were audited, not imported.
No need to attempt a general full amp. Fallback C only if the real-guitar attempt
is unstable, computationally excessive or ineffective; founder accepts sound.

High-Z1M/10n input and buffered Drive pre-gain4..80 (own guitar adaptation,
±4V preamp headroom), Level and isolated1u/100k output coupling. No speaker/cab/
valve/RevivalDRIVE recreation. Begin4x97tap; observe1x/2x/4x alias before any increase.
Expected DC healthy output0,both branches≈0.65mA; fault output≈−2.84mV with
feedback versus−26.1mV without, upper branch off. These are preliminary reduced-
model calculations, not hardware measurements or guaranteed audible improvement.

Predeclare small independent equivalent SPICE gates: DC/transfer within1mV,
short low-amplitude transient peak within5mV; complementary conduction, explicit
feedback-off/on comparison at declared matched nominal output, sensible100/1k/10k
loads, finite/reset/silence/controls and callback guards. Report crossover/alias
actual results, no science threshold relaxation. Nine take03 dry/raw/RMS WAVs;
raw same input/Level, no compensation to fabricate fault. No complete legacy suites.
