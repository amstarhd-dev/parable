# SF-007 permitted real-guitar input

Reuse the same pinned **GuitarJam / Julian-br**, CC0-1.0, as SF-006; original
card retained at `third_party/tape/GuitarJam-README.md`. Exact recording:

https://huggingface.co/datasets/Julian-br/GuitarJam/resolve/2d467bfec90af19301b01123494f4b2ba64c5a3a/guitar_jam/test/050.wav

Revision **2d467bfec90af19301b01123494f4b2ba64c5a3a**, SHA256
**22e7b845fc2d71200c72c13fedbeb5f8ac593e48a01f99875f857e0cc5dfecd0**,
987046 bytes, mono PCM16/44.1 kHz/493501 frames. Author describes clean
Stratocaster -> Scarlett Solo -> Audacity without amp/effects. The pinned public
CC0 card is permission basis; authorship/ownership not independently authenticated.
This is listening audio, not a DSP-code import or license-allowlist exception.

Both versions receive the entire same input at peak .25 normalized and a 2 s
silent tail; no resampling or physical-voltage calibration. Full-band dry/healthy/
worn, actual horn taps and identical drum tap are labeled separately. Synthetic
09/10/11 files are owned transient/steady test signals, explicitly **not guitar**.
RMS matching gains are in renders.csv; no human/founder approval is inferred.
CI checks SHA, includes original/card/provenance/notices and source/compiler
identity. Custom CLI input needs the caller's permission. DSP/tests have no
recording-download dependency; CI download is an explicit, pinned listening step.
