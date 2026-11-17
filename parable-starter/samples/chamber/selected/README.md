# Selected full-Cloud chamber and one return gain fault

Start **healthy_matched.wav / fault_matched.wav**. Then compare healthy_raw and
fault_raw at the SAME playback gain. Same8s take03 plus8s tail; mono48k PCM16.
Matched copies target.05 RMS with separate explicit gains in levels.csv. They
are not LUFS-matched. The raw fault is substantially louder; it is not normalized.

- healthy_wet / fault_wet: wet-only readout at fixed3x gain in both states.
- *_raw_mic / *_raw_speaker / *_raw_return: actual raw microphone, speaker
  excitation and applied64sample-delayed return at unity diagnostic gain.
- healthy_weak / fault_weak: synthetic.005 pluck, bit-identical; no return conduction.
- healthy_strong / fault_strong: same synthetic.15 pluck. Input stops after.5s;
  damaged return sustains a bounded~1.76kHz response. No oscillator/tone track.
- healthy_chord / fault_chord: synthetic two-tone sustained excitation for2s.
- fault_unexcited_zero:12s true silence from startup, including fault gain.
- fault_strong_then_disconnect: same strong probe; return gain ramps to0 at8s,
  preserving the room state and demonstrating recovery.
- *_impulse:0.5 input impulse, wet1. This short impulse stays below the filtered
  mic threshold; the two responses are identical. *_common64 copies use exactly
 64x common offline gain to make them audible. Raw .f32 is little-endian48k
  IEEE float32 for numerical analysis; no gain or header.

Return gain.02→24 is the ONLY difference in each paired configuration. Room,
threshold, filters, headroom, source, wet/volume and readout stay identical. Shared
post-output containment is not reached in any normal deliverable. The fault's
singing continues at the end of files whose return stays connected; file ending
is a recording cut, not DSP recovery. loop_trace.csv samples actual nodes every37
frames; diagnostic WAVs retain every sample at PCM16 precision.

settings.json, levels.csv, SOURCE_COMMIT, SOURCE_SHA256SUMS and root SHA256SUMS
record exact provenance/settings. Preserve AUDIO_NOTICE.md. This is an experimental
return-amplifier hypothesis around a complete borrowed algorithmic room; no measured
chamber/device match, hardware admission or final musical approval is claimed.
