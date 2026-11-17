# Wallace experiment — source, license and changes

Guitar-TECHS, ICASSP2025. Credit Hegel Emmanuel Pedroza Villalobos, Termeh Taheri,
Wallace Abreu, Ryan M. Corey and Iran R. Roman.
https://zenodo.org/records/14963133 ; https://guitar-techs.github.io/ .
**CC-BY4.0**, https://creativecommons.org/licenses/by/4.0/ . Retain this attribution,
source/license links and change notice when redistributing these recordings.

Exact original Player3 take03 DI, stereoPCM24/48k,1406000frames, SHA256
84980b5468f3315171e4e546a1d38d54e4c368ccb5fee72c759e0915f346fad0.
Original source is retained at ../wallace/source-original-stereo24.wav. Archive P3_music.zip,
SHA256033489e22600751fb5a1633e7d856b901c6782e0486fa02135e830780d9dbfe2,
member P3_music/audio/directinput/directinput_03.wav. Same underlying performance
as SF011/14/22, not a newly generated or simulated guitar.

Changes: select9.28–29.28s (frames445440..1405440 exclusive), channel0;
24→16 nearest/ties-away, no resampling/EQ/artificial gaps/fades/time stretching.
This later20s contains actual rhythmic gaps and final sustained decay. The first20s
used in earlier auditions remains reproducible with main tools/klon_input.py;
wallace_input.py reproduces this later excerpt. Original receipt: ../wallace/input-receipt.json records the exact SHA.

C++ renderer applies one common software input gain to0.2 peak, not measured pickup
voltage, then appends9s silence for real processor tails. Source signal/settings
identical across all comparisons. All ten raw fullmix files use output gain1.
Their _matched copies use independently disclosed CONSTANT gains to−23LUFS;
no loudnorm processing, compressor or limiter.05/06/07 _common_gain versions
apply one identical gain. Contribution _listen solos use their own fullmix gain,
never independent solo normalization. Raw originals remain, all monoPCM16/48k.
SF024 adds measured cabinet convolution and clean/parallel routing as recorded in
IR_NOTICE.md/README.md/settings.json. Existing four effects are unchanged.

Hall core: Chris Johnson's MIT Airwindows kCathedral; code license retained at
../../third_party/hall/upstream/LICENSE. Convolution: HiFi-LoFi/FFTConvolver MIT,
Ooura credit/terms at ../../third_party/fftconvolver/. No global repository/code
license is inferred from the audio CC license. FFmpeg is an external measurement
and file-quantization tool, not linked DSP.
