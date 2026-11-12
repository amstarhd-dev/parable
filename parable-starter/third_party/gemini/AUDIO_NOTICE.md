# SF-005 external listening input

`GuitarJam`, Julian-br, revision `2d467bfec90af19301b01123494f4b2ba64c5a3a`,
`guitar_jam/test/050.wav`. The pinned dataset card explicitly declares
**CC0-1.0**: <https://creativecommons.org/publicdomain/zero/1.0/>.
Its complete 493-byte card is retained as `GuitarJam-README.md`; recording/card
URLs and SHA256 hashes are in `audio-provenance.json`.

This is a real clean direct-input electric-guitar recording according to the
author's card, not our recording or a verified analog voltage calibration. The
author describes a Fender Stratocaster neck/middle pickup, Focusrite Scarlett
Solo and Audacity; the source is mono PCM16 / 44.1 kHz. No personal or medical
data. CC0 is the source's published dedication, not an independently authenticated
claim of ownership or a founder musical approval.

No recording is required/downloaded in portable DSP, normal host builds or tests.
Release CI explicitly downloads this one hash-pinned external host input and
publishes the original plus processed A/B WAVs. Reproduction can instead use an
owned mono PCM16 recording via the existing lab reader. No other library, model,
code, source dataset or CC0 code exception is imported. Soundforge's conservative
MIT/BSD-2-Clause **code** import policy is unchanged; this notice records the
separate permission/provenance review of listening data.

Both states receive the identical source, scaled to 0.15 model-V peak and padded
with one second of silence. Raw and whole-file RMS-matched renders disclose
calibration, matching gains and electrical changes. No cabinet, amp, additional
glitch effect, noise, pitch shift or source-performance edit is added.
