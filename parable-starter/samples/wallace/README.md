# The Wallace experiment — same four effects, three routings

Start with the **−23LUFS matched** principal recordings:

1. [A — conventional serial](03_A_serial_lufs.wav)
2. [B — parallel console, static](04_B_parallel_lufs.wav)
3. [C — parallel console, ducked](05_C_dynamic_lufs.wav)

One real guitar performance, same healthy Big Muff/Rotary/Tape/Grand Hall, same
controls and source. Constant-gain loudness matching only; no mastering compressor,
limiter, new effects, artificial guitar/gaps or algorithm rewrites. Musical
superiority is **for listening**, not established by the measured differences.

| Required comparison | Actual recording |
| --- | --- |
|01 Dry guitar, common input calibration|[01_dry.wav](01_dry.wav)|
|02 Same Big Muff alone|[02_muff.wav](02_muff.wav)|
|03 A Serial, raw common gain1|[03_A_serial_raw.wav](03_A_serial_raw.wav)|
|04 B Parallel static, same gain|[04_B_parallel_raw.wav](04_B_parallel_raw.wav)|
|05 C Parallel dynamic, same gain|[05_C_dynamic_raw.wav](05_C_dynamic_raw.wav)|
|06 B wet returns alone|[06_B_wet.wav](06_B_wet.wav)|
|07 C wet returns alone|[07_C_wet.wav](07_C_wet.wav)|
|08 C without return spectral carving|[08_C_no_carving.wav](08_C_no_carving.wav), [LUFS copy](08_C_no_carving_lufs.wav)|
|09 C without ducking|[09_C_no_ducking.wav](09_C_no_ducking.wav), byte-identical to B|

Wet-return files sum the actual weighted rotary+tape+hall contributions. For more
audible isolation, [B common-listen](06_B_wet_common_listen.wav) and
[C common-listen](07_C_wet_common_listen.wav) apply **one identical7.673615 gain**
to both; their relative attenuation is preserved. Original raw files stay unchanged.
Three unweighted native wet stems are also included, explicitly diagnostics.

All output monoPCM16/48k, **29s =20s actual performance+9s real effect tails**.
Listen around3.88–4.26s and14.22–14.90s for natural playing gaps,15–20s for the
final sustained/decaying phrase,20–29s for the tail. Source is9.28–29.28s from the
same Guitar-TECHS take03 previously used, not a different or generated performance.
Full exact source, prepared excerpt, attribution and reproducible crop are retained.

Settings: healthy Muff Sustain.65/Tone.5/Volume.65,4x; rotary command.7; healthy
tape1.2s/.3s delay/.32 feedback/2.4 drive; healthy hall decay1. Serial wet mixes
.35/.25/.20. Parallel core.39/rotary.21/tape.20/hall.20, sends1; clean blend0.
Tape/Hall return HPFs120/180Hz. C only: input envelope10ms attack/250ms release,
0..4dB Tape+Hall ducking. Independent clean/gains/sends/attack/release/depth exposed
by renderer flags. No effect's internal DSP is changed.

Source/tone/model limitations and exact values are in [settings.json](settings.json),
[levels.csv](levels.csv), [analysis.json](analysis.json), [ducking.csv](ducking.csv),
[AUDIO_NOTICE](AUDIO_NOTICE.md), input receipts and SHA/source identities.
Loudness is FFmpeg ebur128 mono BS1770 integrated gating (meter0.1LU resolution),
not a guarantee of equal subjective timbre. Raw references are intentionally quieter
than the matched listening copies. No physical amp/cab recording is inferred.

[Full report and reproducible commands](../../docs/SF023_REPORT.md).
Only mixing infrastructure has new ARM evidence. **Entire graph not admitted to
Daisy; no device timing/boot/audio claim.** No Engine/patch/correspondence integration.
