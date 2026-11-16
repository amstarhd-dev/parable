# Gemini coupled-room listening files — experimental

Start with **healthy_matched.wav / fault_matched.wav**. These contain the same
8s guitar performance followed by4s tail, mono48k PCM16. The separate matched
copies target0.05 RMS; gains are explicit in levels.csv. They are not LUFS-matched.

- `00_dry_raw.wav`: common calibrated input, including tail silence.
- `healthy_raw.wav / fault_raw.wav`: identical input/gain/aperture.85/wet.65;
  sole change is the B-return-to-A feedback pickup sign. No normalization.
- `healthy_wet_only.wav / fault_wet_only.wav`: same fixed wet readout, no dry.
- `healthy_room_A/B.wav`, `fault_room_A/B.wav`: isolated room returns, same gain.
- `coupling_off_A.wav / coupling_off_B.wav`: aperture0, input only in A;
  B is exactly silent. The same room B is active when aperture opens.
- `impulse_healthy*.wav / impulse_fault*.wav`: four-second0.5 impulse, wet1;
  mono plus isolated A/B outputs, no gain matching.
- `input_take03_20s.wav`: unchanged approved prepared source for reproduction.

These are19 rendered WAVs plus the20s source. All guitar renders use first8s,
common source peak0.15; raw mono=.35*dry+.65*.5*(A+B). Diagnostic tap gain differs
from mono only by that documented readout; there is no per-room normalization.
See settings.json, levels.csv, source-receipt.json and AUDIO_NOTICE.md for settings,
gains, exact source/CC-BY4.0 credit. SOURCE_COMMIT identifies producing code,
SOURCE_SHA256SUMS its sources, SHA256SUMS this package.

Numerical distinction does not prove that listeners recognize two spaces or
prefer the fault. No founder musical acceptance, architectural room calibration,
Seed3 execution or production integration is claimed. See ../../docs/SF020_REPORT.md.
