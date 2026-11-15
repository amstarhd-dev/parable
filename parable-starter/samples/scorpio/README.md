# SF-017 — listen first

Download a WAV using GitHub's **Download raw file** button. CC-BY credit and
changes: [AUDIO_NOTICE.md](AUDIO_NOTICE.md). Founder listening approval pending.

| Same source, same Level | Healthy | R3 drift only |
|---|---|---|
| Restrained, RMS matched | [low](low_healthy_matched.wav) | [low fault](low_R3_10k_matched.wav) |
| Aggressive, RMS matched | [high](high_healthy_matched.wav) | [high fault](high_R3_10k_matched.wav) |
| Restrained, actual relative level | [raw](low_healthy_raw.wav) | [raw fault](low_R3_10k_raw.wav) |
| Aggressive, actual relative level | [raw](high_healthy_raw.wav) | [raw fault](high_R3_10k_raw.wav) |

[External dry/common](00_external_dry_common.wav) and
[dry with declared limited matching gain](01_external_dry_matched.wav).
The **four processed matched files** reach0.07 RMS; dry matching is capped16×
and reaches0.01223 RMS, so dry is quieter. RMS is not LUFS/perceived matching.
Raw files have no makeup. Exact float gains and levels are in[renders.csv](renders.csv),
within one PCM16 quantization step of the delivered WAVs. First8s of Guitar-TECHS
Player3 take03 plus1s tail,48kHz mono PCM16; common0.03V model input peak,
Gain0/1, Low Cut0.35, Level0.8. Fault onlyR3=5.6kΩ→10kΩ.
No measured pickup calibration, physical-pedal fidelity or human audition claim.

`SOURCE_COMMIT` identifies the producing code (later commits may only package
results/docs/workflows). `SOURCE_SHA256SUMS` authenticates exact renderer/kernel
and reused utilities relative to`parable-starter/`; `SHA256SUMS` authenticates
these original outputs and receipts. `first-audio-time.json` records the earlier
19½-minute prototype milestone (193-tap filter,0.12V/Gain0.1/0.9); **these final
files** use the97-tap kernel and settings above, not that earlier producing code.

From repository root, C++17/CMake/Python3 standard library (initial archive129.5MB):

```sh
cmake -S parable-starter -B /tmp/sf017-release -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/sf017-release --target parable_scorpio_lab -j2
mkdir -p /tmp/sf017-input
curl --fail --location https://zenodo.org/api/records/14963133/files/P3_music.zip/content --output /tmp/sf017-input/P3_music.zip
python3 parable-starter/tools/klon_input.py --archive /tmp/sf017-input/P3_music.zip --output /tmp/sf017-input
/tmp/sf017-release/parable_scorpio_lab --input /tmp/sf017-input/take03-20s-mono16.wav --output /tmp/sf017-new-audio
```

Output must be a **new directory**. Input/archive checksums are enforced by the
reused input tool and recorded in`source-receipt.json`. Native renderer resets
and rerenders internally, requiring bit-exact output before accepting each file.
Host timings vary with load/compiler; other receipts and WAVs are deterministic
on the reported host/toolchain, not promised bit-identical across all libm/CPUs.
