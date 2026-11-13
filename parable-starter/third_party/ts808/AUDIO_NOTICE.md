# NEW SF-010 guitar source — Guitar-TECHS take06

Credit Hegel Emmanuel Pedroza Villalobos, Termeh Taheri, Wallace Abreu,
Ryan M.Corey and Iran R.Roman. *Guitar-TECHS: An Electric Guitar Dataset Covering
Techniques, Musical Excerpts, Chords and Scales Using a Diverse Array of Hardware*,
ICASSP2025. Dataset record14963133 v1, Player3 musical performance06, direct-input
branch; not an amp/microphone recording or GuitarJam050/Muff take11.

Record/license: https://zenodo.org/records/14963133
Setup: https://guitar-techs.github.io/
License: **Creative Commons Attribution4.0 International**,
https://creativecommons.org/licenses/by/4.0/ . Preserve credit/source/license and
this transformation notice with redistributed audio.

The source authors document a guitar split to direct interface versus a separate
miked amplifier. Player3 uses SireT7/bridge-neck/UMC202HD. These are source-declared
facts, not independently calibrated hardware or agent/founder musical approval.
The record offers only Player3's music archive; this is a genuinely different
performance, not a claim of a different player or dataset. Pleasantness remains
a listening decision.

Archive https://zenodo.org/api/records/14963133/files/P3_music.zip/content
129505089bytes, published MD5 `071ba80aecf00f4a31fbd167b3f22198`, verified SHA256
`033489e22600751fb5a1633e7d856b901c6782e0486fa02135e830780d9dbfe2`.
Member `P3_music/audio/directinput/directinput_06.wav`: stereoPCM24/48k,
1536000frames/full32s,9216690bytes, SHA256
`2fd88dc725f475e13686f50ce6c88e7fed3d6b635b62147a69d42e29a5dd6254`.
The exact full original is retained in the listening ZIP.

`tools/ts808_input.py` verifies the original/checksums/format and sample-identical
channels. Select channel0, take **first26s**, quantize24→16bit nearest/ties-away,
without resampling, EQ, dynamics or gain. Excerpt1248000frames, SHA256
`b1162cf7dfdd5921638e403581541c36c2de73022f3bffd0e111e66302048364`.
Explicit excerpt preserves the unchanged shared PCM helper's30s bound and leaves
2s tail. No old-source rename/resample; no original recording silently altered.

Runner applies the same global peak calibration.2modelV to dry/healthy/fault,
then2s silence:1344000frames/28s. Rawgain1; separate bounded offline RMS matching
and actual gains/crest are disclosed. Softwarevolts/jackVolts÷4 are not a measured
pickup/Seed3 interface. Controlled attacks/notes/sine/triangle are owned synthetic
signals, not guitar. Arbitrary CLI --input does not imply this recording or rights.

Code eligibility remains MIT/BSD-2-Clause only; CC-BY audio permission is separate
data attribution, not an import-policy exception or a global Parable license.
