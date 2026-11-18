# SF025 — Virgo Tiled Room

**[Download the listening ZIP](SF025-listening.zip)**, unzip and play locally.

| Start here | Description |
|---|---|
| [Healthy mono](healthy_matched.wav) | Compact, bright, static room;65% dry retained |
| [Fault mono](fault_matched.wav) | Same input/settings, one diffuser coefficient changes |
| [Healthy stereo](healthy_stereo_matched.wav) | Actual two-channel output |
| [Fault stereo](fault_stereo_matched.wav) | Same dual-output network |
| [Existing Room A comparison](comparison_roomA_matched.wav) | Exact prior Parable room; comparable decay and wet strength |
| [Healthy wet only](healthy_wet_listen.wav) | Diagnostic, fullmix listening gain |
| [Fault wet only](fault_wet_listen.wav) | Diagnostic, fullmix listening gain |
| [Healthy transient](transient_healthy_common_listen.wav) | Three short identical clicks, shared gain |
| [Fault transient](transient_fault_common_listen.wav) | Hear density and persistent modal ringing |

Fullmix matched files use−23LUFS, constant scalar gains only. `_raw.wav` files
retain the same native gain1. Healthy/fault share input, geometry, filtering,
decay control and wet/dry setting; only the right-tank second diffuser feedback
coefficient changes .5→.94. Wet-only files are separate diagnostics; principal
mixed recordings retain65% unprocessed dry guitar.

Existing SF020 comparison isolates Room A with aperture0; native nominal RT60.65s,
measured≈.558s versus Tiled≈.583s. Its wet return is scaled2.6804× OFFLINE to match
healthy Tiled wet RMS before the same.65dry/.35wet mix; unscaled native wet also
retained. This isolates character without making the quieter old return lose by
level. All scalar gains/meter readings appear in matching.json.

The healthy-only milestone was delivered in commit c5a0726 BEFORE fault work.
Founder response: “Promising—continue.” Healthy raw waveform remains unchanged.
Fault musical approval, distinct ceramic personality and PCM70 equivalence remain
unverified by human comparison. This is an independent Dattorro-family character
study, not proprietary Lexicon/Eos firmware or a physical bathroom simulation.

[AUDIO_NOTICE](AUDIO_NOTICE.md) records Guitar-TECHS credits, CC BY4.0, exact
performance/crop and changes. Impulses, native floats, source, settings, numerical
reference, raw/matched WAVs and hashes remain in Git. The ZIP prioritizes listening
files and notices; reproducible native float diagnostics stay beside it.
[Report/tests/resources/reproduction](../../docs/SF025_REPORT.md).
