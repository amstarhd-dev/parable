# SF-018 — listen

Download WAVs with GitHub's **Download raw file** button.
[CC-BY4.0 credit and modifications](AUDIO_NOTICE.md).

| Distortion | Healthy, matched | C4 open, matched | Raw, same calibration/Level |
|---|---|---|---|
| Low0.15 | [healthy](low_healthy_matched.wav) | [open](low_C4_open_matched.wav) | [healthy](low_healthy_raw.wav) / [open](low_C4_open_raw.wav) |
| High0.9 | [healthy](high_healthy_matched.wav) | [open](high_C4_open_matched.wav) | [healthy](high_healthy_raw.wav) / [open](high_C4_open_raw.wav) |

[Shared dry](00_dry_common.wav). Nine files total: same Guitar-TECHS Player3 take03,
first8s plus1s tail,48kHz mono PCM16; common0.04V model input peak, Tone0.5,
Level0.8, raw jackVolts/2. Only C4 changes250pF→open. Matched files each0.07 RMS;
separate offline gains are explicit in`renders.csv`, not LUFS or hidden processing.
Founder musical approval and perceptual distinctness from other pedals are pending.

`SOURCE_COMMIT`/`SOURCE_SHA256SUMS` identify the actual renderer/kernel/helpers.
`SHA256SUMS` authenticates this package; `source-receipt.json` identifies the
approved archive/member/preparation. Native rendering internally requires an
exact reset/rerender match. Final quality8x193tap; first WAV milestone10min43.028s
was the earlier4x97tap prototype at original commit63f376b (reproducible rebased equivalent f640d24). These final files use the
producing commit recorded here. Host timings vary; no cross-platform bit identity
or real pickup/codec/1978-pedal calibration is promised.

From repository root (C++17/CMake/Python3; initial archive129.5MB):

```sh
cmake -S parable-starter -B /tmp/sf018-release -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/sf018-release --target parable_capricorn_lab -j2
mkdir -p /tmp/sf018-input
curl --fail --location https://zenodo.org/api/records/14963133/files/P3_music.zip/content --output /tmp/sf018-input/P3_music.zip
python3 parable-starter/tools/klon_input.py --archive /tmp/sf018-input/P3_music.zip --output /tmp/sf018-input
/tmp/sf018-release/parable_capricorn_lab --input /tmp/sf018-input/take03-20s-mono16.wav --output /tmp/sf018-new-audio
```

Output must be a new directory. Input tool verifies the pinned archive. Published
source-preparation digest is75ae5e73c72e6774fb75e6318dc91ba66347fb65290949e947144033eb648f52.
`comparison.json` uses PCM16 first8s, equal Hann window, NumPy2.3.5 rFFT power
summed across each stated band. Reproduce for either matched pair with:

```python
import wave, numpy as np
from pathlib import Path
p=Path('parable-starter/samples/capricorn')
def spectrum(name):
    with wave.open(str(p/name)) as w:
        x=np.frombuffer(w.readframes(384000),dtype='<i2').astype(float)/32768
    return abs(np.fft.rfft(x*np.hanning(len(x))))**2
h=spectrum('low_healthy_matched.wav'); f=spectrum('low_C4_open_matched.wav')
hz=np.fft.rfftfreq(384000,1/48000); band=(hz>=6000)&(hz<12000)
print(10*np.log10(f[band].sum()/h[band].sum()))
```

NumPy is optional for this analysis only; build/tests/reference generation use
standard Python. Band energy and waveform difference are not hearing verdicts.
