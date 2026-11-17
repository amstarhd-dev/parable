# SF-022 guitar recording

Guitar-TECHS, ICASSP2025; Hegel Emmanuel Pedroza Villalobos, Termeh Taheri,
Wallace Abreu, Ryan M. Corey and Iran R. Roman. Source:
https://zenodo.org/records/14963133 and https://guitar-techs.github.io/ .
Creative Commons Attribution4.0 International:
https://creativecommons.org/licenses/by/4.0/ . Retain credit/license/change notice.

Player3 take03 direct input, the established SF011/SF014/SF021 performance.
Archive `P3_music.zip`, SHA256
`033489e22600751fb5a1633e7d856b901c6782e0486fa02135e830780d9dbfe2`;
member `P3_music/audio/directinput/directinput_03.wav`, SHA256
`84980b5468f3315171e4e546a1d38d54e4c368ccb5fee72c759e0915f346fad0`.
Main's `tools/klon_input.py` selects channel0/first20s and quantizes24→16
nearest/ties-away, no EQ/gain/resampling. Prepared1920044bytes, SHA256
`75ae5e73c72e6774fb75e6318dc91ba66347fb65290949e947144033eb648f52`.
The committed prepared input is exact; no unmerged effect DSP is imported.
Original archive can be fetched/reverified by that existing tool; full stereo
original is not duplicated in this package.

This laboratory takes first8s, adds8s tail, processes the same native C++ hall,
retains raw0.65dry+0.35wet and wet-only files, and separately applies whole-file
RMS gains with receipts. No pickup/codec voltage calibration. Chord/impulse/zero
probes are synthetic and explicitly labeled; they do not replace real guitar.
Stereo files preserve actual core channels; mono files use their arithmetic sum.
Reset demonstration stops/resets at4s with an explicit external5ms output fade.
