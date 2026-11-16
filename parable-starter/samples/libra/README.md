# SF-013 — Libra listening

Download WAVs using GitHub's **Download raw file** button.
[CC-BY4.0 credit and processing](AUDIO_NOTICE.md).

| Drive | Healthy, RMS matched | R_B1 drift, RMS matched | Actual relative levels |
|---|---|---|---|
| Low0.1 | [healthy](low_healthy_matched.wav) | [fault](low_RB1_270k_matched.wav) | [healthy raw](low_healthy_raw.wav) / [fault raw](low_RB1_270k_raw.wav) |
| Higher0.65 | [healthy](high_healthy_matched.wav) | [fault](high_RB1_270k_matched.wav) | [healthy raw](high_healthy_raw.wav) / [fault raw](high_RB1_270k_raw.wav) |

[Shared external dry](00_dry_common.wav). Nine files total, same Player3 take03
first8s plus1s tail,48kHz mono PCM16. Common0.04V model input peak and Level0.8.
Raw jackV/4 has no makeup; external dry is bypass-equivalent inputV/4. Matched pairs
reach0.07 RMS with separately disclosed gains, not LUFS/perceived loudness matching.
Physical driver rail counts are normal overdrive, not PCM clipping or safety fallback.

Only R_B1 changes27k→270k. Both opposing branches, closed feedback and all other
components remain. The fault's relative crossover effect is strongest at low
levels; feedback reduces it, not eliminates it. Human musical approval remains
pending. This is a declared fixed-V_BE reduction, not a calibrated commercial amp.

`SOURCE_COMMIT`/`SOURCE_SHA256SUMS` identify the actual native renderer and reused
helpers; `SHA256SUMS` authenticates package files. Native rendering requires exact
reset/repeat equality. `first-audio-time.json` records17min32.967s from start using
prototypefb93e30 (higher Drive0.9); final code corrects diagnostic metadata and
selects0.65 for the less compressed higher-drive example. Source receipt identifies
the unchanged approved archive/member/preparation; no physical codec calibration.

Repository-root reproduction (C++17/CMake/Python3, archive129.5MB):

```sh
cmake -S parable-starter -B /tmp/sf013-release -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/sf013-release --target parable_libra_lab -j2
mkdir -p /tmp/sf013-input
curl --fail --location https://zenodo.org/api/records/14963133/files/P3_music.zip/content --output /tmp/sf013-input/P3_music.zip
python3 parable-starter/tools/klon_input.py --archive /tmp/sf013-input/P3_music.zip --output /tmp/sf013-input
/tmp/sf013-release/parable_libra_lab --input /tmp/sf013-input/take03-20s-mono16.wav --output /tmp/sf013-new-audio
```

Output must be a new directory. Input tool checks the pinned archive; prepared
SHA75ae5e73c72e6774fb75e6318dc91ba66347fb65290949e947144033eb648f52.
Host timings vary; exact bytes across different compilers/libm/CPUs are not promised.
