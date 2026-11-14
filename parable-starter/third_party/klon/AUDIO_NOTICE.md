# SF-011 NEW guitar source — Guitar-TECHS take03

Credit Hegel Emmanuel Pedroza Villalobos, Termeh Taheri, Wallace Abreu,
Ryan M. Corey and Iran R. Roman. Guitar-TECHS, ICASSP2025.
Zenodo https://zenodo.org/records/14963133 (metadata revision12 observed),
https://guitar-techs.github.io/ . Source record grants **Creative Commons
Attribution4.0 International**, https://creativecommons.org/licenses/by/4.0/ .
Preserve this credit, source, license and change notice with redistributed audio.
No repository-wide code license or code-policy exception is inferred.

Player3 musical performance03 direct-input, not an amp recording, GuitarJam050,
Muff take11 or TS808 take06. Same player/dataset, genuinely different performance;
no asserted different player or founder pleasantness approval. Source-declared
Player3 setup: SireT7/bridge-neck/UMC202HD; not our hardware measurements.

Archive https://zenodo.org/api/records/14963133/files/P3_music.zip/content
129505089bytes, published MD5`071ba80aecf00f4a31fbd167b3f22198`, SHA256
`033489e22600751fb5a1633e7d856b901c6782e0486fa02135e830780d9dbfe2`.
Member `P3_music/audio/directinput/directinput_03.wav`: exact stereoPCM24/48k,
1406000frames(~29.291667s),8436690bytes, SHA256
`84980b5468f3315171e4e546a1d38d54e4c368ccb5fee72c759e0915f346fad0`.
The exact full original is retained in the listening artifact.

`tools/klon_input.py` verifies archive/member/format/channel identity, selects
channel0, takes first20s and quantizes24→16 nearest/ties-away. No resampling,
EQ, dynamics or gain. Prepared960000frames,1920044bytes, SHA256
`75ae5e73c72e6774fb75e6318dc91ba66347fb65290949e947144033eb648f52`.
The original remains untouched. Explicit excerpt plus2s circuit tail fits the
unchanged shared30s PCM helper. Record ID/archive SHA identify content; no
invented dataset semantic version or DOI is claimed.

The runner's main pairs all use the same.2modelV peak input calibration and
jackVolts/8 software output headroom. Dry/input WAVs also use Volts/8. These are
software units, not pickup/codec/Seed3 calibration. Low/medium/high Gain and
Output settings, raw levels, exact bounded offline RMS-match gains and crest
are in receipts. Extra low/high guitar probes use.04/.4modelV at medium controls.
Analysis path WAVs are loaded feedback-voltage contributions before Tone/FIR,
not independently isolated sources or final output. Controlled attacks/notes/
impulse-release/chord/balanced sine are owned synthetic signals, not recordings.
Arbitrary --input implies no rights or attribution to this recording.
