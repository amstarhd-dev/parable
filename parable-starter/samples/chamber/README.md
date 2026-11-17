# SF021 chamber candidates — Gate1, experimental and not approved

Listen to candidates/cloud/compact_comparable_mix.wav and
candidates/signalsmith/compact_comparable_mix.wav. Both are actual full upstream
engines, same take03/peak.15,35%dry+65%RMS-calibrated wet return. Calibration is
separate and explicit; original wet level is retained in compact_upstream_raw.wav.
compact_matched.wav is wet-only at shared.04 RMS. Each compact receipt records
input gain, raw RMS/peak, calibration, full-process host time and observed memory.

Both engines also have default/preset recordings, matched copies and impulse
tails. Cloud preset is its original DarkPlate with original dry level included;
Cloud default keeps DarkPlate parameters but mutes dry. Signalsmith default is
wet-only default and preset is wet-only ambient (nominal RT60~25.5s;14s record
truncates that long tail). These are background references, not chamber presets.
Compact retunings use only upstream controls and nominal~1.2s decay,120/6500Hz
coloration; see source and ADR for exact parameters. No claim of equal measured
RT60 or physical chamber dimensions. No source rewriting to manufacture a reverb.

Original Cloud ClearBuffers does not reset all positions/phases/filter states:
its impulse follows14s guitar and then buffer clear. Signalsmith uses full reset.
Fresh process rendering with fixed seed is separately checked for repeatability.
The source is the unchanged prepared20s take03; only first8s is used, then6s tail.
Preserve AUDIO_NOTICE.md. Founder sound choice remains pending.
