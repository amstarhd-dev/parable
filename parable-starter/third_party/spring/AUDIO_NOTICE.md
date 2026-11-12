# SF-008 permitted real-guitar input

Same CC0 GuitarJam / Julian-br input as SF-006/007, pinned card at
`third_party/tape/GuitarJam-README.md`. The source describes clean Stratocaster
-> Scarlett Solo -> Audacity, no amp/effects. Public CC0 card is permission basis;
authorship/ownership has not been independently authenticated.

https://huggingface.co/datasets/Julian-br/GuitarJam/resolve/2d467bfec90af19301b01123494f4b2ba64c5a3a/guitar_jam/test/050.wav

Revision **2d467bfec90af19301b01123494f4b2ba64c5a3a**, SHA256
**22e7b845fc2d71200c72c13fedbeb5f8ac593e48a01f99875f857e0cc5dfecd0**,
987046 bytes, mono PCM16/44100 Hz/493501 frames. Card SHA256
**e6df26337b6307b41b58bdb0f5d9d7e185f5784028f5b2dcba13d2f21e7e78f8**.

Both spring configurations receive the entire same source at **peak.8 normalized**
(software gain4.62335097002), NOT calibrated physical voltage. This deliberately
differs from prior tape/rotary peak.25; no resampling, add6s silence. Six primary
files have758101 frames each. Dry/healthy/fault are raw common gain1 and separately
whole-file RMS matched at -23dBFS subject to.89 peak ceiling; not LUFS matching.
Weak/strong/impulse files are explicitly OWNED synthetic mechanical probes,
not guitar. Additional *_common64.wav probe pairs use the SAME offline64 gain
across weak/strong/impulse, retaining relative levels. No human/founder listening approval is inferred from rendering.

CI verifies input SHA and includes original/card/notices/provenance, exact source
and compiler identity, all WAV hashes/receipts. Caller-supplied CLI input needs its
own permission; tests need no external recording download. No other tank recording
or impulse response imported. Artifact expires after30days; rerun CI to regenerate.
