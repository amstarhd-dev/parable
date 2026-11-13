# SF-009 NEW clean guitar DI — Guitar-TECHS

**Credit:** Hegel Emmanuel Pedroza Villalobos, Termeh Taheri, Wallace Abreu,
Ryan M. Corey and Iran R. Roman. *Guitar-TECHS: An Electric Guitar Dataset
Covering Techniques, Musical Excerpts, Chords and Scales Using a Diverse Array
of Hardware*, ICASSP2025, dataset v1. Player3 performance11, classified as a
musical excerpt, direct-input branch. No amp/microphone file selected.

Source: https://doi.org/10.5281/zenodo.14963133
Setup: https://guitar-techs.github.io/
License: **Creative Commons Attribution4.0 International**
https://creativecommons.org/licenses/by/4.0/

The primary Zenodo record explicitly licenses the data CC-BY4.0. The authors'
setup splits guitar output to the interface and a separate miked amplifier.
Player3: Sire T7, bridge-neck pickup, UMC202HD interface. These are source-declared
facts, not independent authentication/calibration or an agent musical rating.
Attribution/source/license and this transformation notice must travel with audio.

Pinned archive:
https://zenodo.org/api/records/14963133/files/P3_music.zip/content
129505089bytes, published MD5 **071ba80aecf00f4a31fbd167b3f22198**, SHA256
**033489e22600751fb5a1633e7d856b901c6782e0486fa02135e830780d9dbfe2**.
Member **P3_music/audio/directinput/directinput_11.wav**,
6084690bytes, stereo PCM24/48000Hz/1014000frames (21.125s), SHA256
**0842bfb4e73142205123d6d362ff8f007e277ce022a4e40dd70b824ecb7543d8**.

Original stereo channels are sample-identical. `tools/muff_input.py` keeps channel0,
converts24->16bit by nearest/ties-away integer quantization, retains full duration;
no trimming, resampling, gain, EQ, dynamics or effects. Prepared mono SHA256
**f0299640eaebf29328716567fbf60e0a5639e916ed5d2f81bf37b8ef032ab2bc**.
Exact original and conversion receipt are in the listening artifact.

Lab then applies the SAME global input-peak calibration.2 modelV to dry/healthy/
fault and appends2s silence:1110000frames/file. No measured physical pickup/Daisy
calibration. Raw gain1 and separate RMS matching (maximum16x) with all gains/crest
reported. Controlled two-attack/probe/long-note signals are ORIGINAL SYNTHETIC,
not guitar. Caller-provided CLI input has no inferred rights or recording origin.

This is a genuinely different performance/dataset/player from rejected
GuitarJam050. No old recording is included in the new Muff ZIP or renamed as new;
old merged goldens/archives remain unchanged. Soundforge code-import license policy
is unchanged; CC-BY recording permission is independently documented data rights.
