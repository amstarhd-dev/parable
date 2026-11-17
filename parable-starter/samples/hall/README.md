# SF-022 — Leo Grand Hall listening experiment

Start [healthy matched](selected/healthy_matched.wav) and
[damaged matched](selected/fault_matched.wav). Same recorded take03 and settings;
only final reflecting-plane coherence changes0→0.98. RMS copies use explicit gains
in [levels.csv](selected/levels.csv); its RMS/peak/mean columns describe the raw signal before that gain. these are not perceptual/LUFS normalization.

Hear [healthy stereo](selected/healthy_stereo.wav) / [damaged stereo](selected/fault_stereo.wav)
for actual spatial output, or raw mono [healthy](selected/healthy_raw.wav) /
[damaged](selected/fault_raw.wav). Same0.65dry+0.35wet and unmodified source level.
Wet-only files and labeled impulse/chord/weak/silence/reset probes are in selected/.
No extra oscillator/howl or separate resonator is mixed into the fault.

Borrowed comparison: complete original [kCathedral](candidates/kcathedral/matched.wav)
and [Verbity](candidates/verbity/matched.wav), measured decay4.89/4.95s. Original
[kCathedral wet](candidates/kcathedral/wet.wav) and adopted
[healthy wet](selected/healthy_wet.wav) are byte-identical. The raw/matched/wet files
and original float IRs retain exact original processing; originals are not a new
reverb pretending to be a source reference.

8s guitar +8s tail,48kHz PCM16. Stereo files are two-channel; ordinary others mono.
`*.f32` are little-endian float32 mono48k impulse responses, not WAV containers.
`reset_stopped_faded.wav` uses a stopped reset at4s and explicit external5ms output
fade; it does not prove a native live clickless reset. The source has intentional
near-zero noise/dither; PCM silence can quantize this to zero.

[Credit/license/change notice](AUDIO_NOTICE.md), [settings](comparison.json),
source commit/digests and package SHA256SUMS accompany downloads. The source is
CC-BY4.0 real guitar; synthetic chord/impulse/zero files are separately labeled.

No founder musical approval, physical hall calibration or Seed3 runtime measurement
is claimed. The weak-state fault is quieter in absolute terms but changes coherence
at all levels; its "largely normal" acceptability requires listening. No prior
ambience implementation exists in current main for a substantiated quality A/B.
See [report](../../docs/SF022_REPORT.md) and [ADR022](../../docs/adr/ADR-022-leo-grand-hall.md).
