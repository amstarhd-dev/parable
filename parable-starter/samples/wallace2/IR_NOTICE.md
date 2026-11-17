# Measured cabinet recordings — CC BY 4.0

**jesterdyne**, March18,2011:

- “Jensen Cab SM57 center.wav”: https://freesound.org/people/jesterdyne/sounds/116743/
- “Engl Celestion V30 SM57 center-01.wav”: https://freesound.org/people/jesterdyne/sounds/116735/

Both licensed **Creative Commons Attribution4.0 International** by their original
author: https://creativecommons.org/licenses/by/4.0/ . Credit applies to these
recordings and derived cabinet-processed listening files. No endorsement implied.
Author describes measured guitar-cabinet IRs, SM57 center on-axis to cone; the
latter filename and generic H&K description conflict. No verified open/closedback,
speaker model beyond filename, sweep/calibration or exact cabinet dimensions.

Lossless byte source: OpenSauce/rustortion commit
1522f4d64f520bd665cfb098601a2cbf0bcae0d8:
`impulse_responses/Jesterdyne/Jensen/sm57-center.wav` and
`impulse_responses/Jesterdyne/Engl/sm57-center-01.wav`.
Identical copies cross-checked against Raven-deadwire/SpectralForge-Chimera-Amp-Matrix
b900c9f7f028a74446ec65f11aa8a2e435c12ecb `Assets/IRs/guitar_jensen_sm57.wav` /
`Assets/IRs/guitar_v30_sm57.wav`, its docs/THIRD_PARTY_NOTICES.md confirming
unchanged originals. Rights derive from original Freesound pages, not either
repository's code license. No Rust/JUCE/Chimera source imported.

`irs/*-original.wav`: unchanged44100Hz mono24bit originals. Changes in this study:
FFmpeg resample48000Hz float32, whole-response scalar unity-maximum-magnitude
normalization; full lengths and original onset/phase retained. Exact argv, gains
and SHA256 in irs/preparation.json. Prepared kernels stored little-endian IEEE
float32 without WAV headers. FFTConvolver automatically omits trailing samples
below1e-6: Jensen none; V30 last89samples, absolute coefficient sum2.793e-5.
Independent references compare against the **full** prepared IR, exposing that
small approximation. No minimum-phase transform, onset trimming or synthesized
cabinet. No assertion of physical capture independently repeated by Parable.
