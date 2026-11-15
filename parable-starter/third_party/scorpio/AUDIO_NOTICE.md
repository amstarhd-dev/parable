# SF017 listening source and changes

Credit Hegel Emmanuel Pedroza Villalobos, Termeh Taheri, Wallace Abreu,
Ryan M. Corey and Iran R. Roman. Guitar-TECHS, ICASSP2025.
Source https://zenodo.org/records/14963133 and https://guitar-techs.github.io/ .
Creative Commons Attribution4.0 International:
https://creativecommons.org/licenses/by/4.0/ . Preserve this notice with audio.

Player3 performance03 direct-input. Archive P3_music.zip SHA256
033489e22600751fb5a1633e7d856b901c6782e0486fa02135e830780d9dbfe2;
member P3_music/audio/directinput/directinput_03.wav original SHA256
84980b5468f3315171e4e546a1d38d54e4c368ccb5fee72c759e0915f346fad0.
Existing tools/klon_input.py prepares first20s/channel0/PCM24->16 nearest,
ties away, no resampling/EQ/gain; prepared SHA256
75ae5e73c72e6774fb75e6318dc91ba66347fb65290949e947144033eb648f52.
SF017 uses first8s of that preparation, calibrates peak0.12modelV, adds1s silence,
and renders the same source through both modes at identical Level. Raw jackVolts/8
and separate offline RMS matching are explicit in settings/renders.csv; not LUFS.
Software units are not measured pickup/codec calibration. The dry file is external
DI with the same software calibration/output divisor. No source ownership beyond
CC-BY rights, physical-pedal match or founder approval is implied.
