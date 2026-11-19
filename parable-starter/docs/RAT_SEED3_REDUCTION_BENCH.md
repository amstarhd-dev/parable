# Seed3 RAT downsizing: bounded, reversible experiment

Branch `bench/seed3-rat-1x-cycles`, draft PR81, 2026-11-19.
**Do not merge or treat these as approved DSP changes.** Normal builds continue to use 4x/193.

## What the compiler creates

| `FIRST_BOOT_IMAGE` | RAT configuration | Purpose |
|---|---|---|
| `rat_composite` | 4x/193; full-precision preamp/clip | Hardware control |
| `rat_composite_97` | 4x/97; original double clip | FIR saving alone |
| `rat_composite_97_floatclip` | 4x/97; single-precision clip state/calculation | Separate FP32 FPU hypothesis |
| `rat_composite_1x` | 1x, no FIR | Upper bound on oversampling cost; not a quality candidate |
| `mono_bypass` | None | Known timing floor |

All images retain the *same* Gain(1.8) → Rat(.35/.5/.7) → Lowpass(1800Hz) graph, output trim .8, 48k/48-frame callback, and DWT instrumentation. Firmware banners uniquely identify the variant; never infer it from the filename alone.

Build in the existing pinned **Linux x86_64 CI** job (`python3 parable-starter/firmware/build.py`). It packages the images as a **first-boot kit** and inspects actual ELF/memory sections. This is not a macOS-native ARM build.

## Before board testing: host differential gate

From repo root, with Python3 and a C++17 compiler:

```sh
python3 parable-starter/tools/rat_candidate_audit.py
```

It compiles and runs the *actual repository source* for 4x/193, 4x/97 and 4x/97/floatclip. Eight synthetic 2-second stimuli: decaying partials, low-drive chord, 3517Hz high-gain sine, 7971Hz low-volume sine, clipped pulses, wideband pseudorandom, positive diode disconnected, both diodes disconnected. Checks finite output, correct length, and an FP32 clipping RMS divergence limit of 0.05% across all eight tests; printed 97/193 differences are diagnostic and **not subject to a permissive fidelity pass**. The 97-tap output has 24 **base-rate samples less group delay** and is aligned before waveform comparison. No subjective fidelity or alias rejection is certified.

Local prior source-transcription results (G++/x86; independent testing of the *new branch source* awaits CI):

| Scenario | 4x97 vs 4x193, time aligned | 4x97 float clip vs 4x97 double clip |
|---|---:|---:|
| Guitar-like pluck | .0057% | .0038% |
| Low drive chord | .0013% | .0064% |
| 3517Hz high gain | .5893% | .0020% |
| 7971Hz low volume | .1659% | .0021% |
| Hard pulses | .5497% | .0103% |
| Wideband | 4.5103% | .0021% |
| One diode open | .4679% | .0311% |
| Both diodes open | .6387% | .0112% |

Values are RMS(error)/RMS(reference) over samples 5000:95000 of each scenario. An 8-test screen is not a listening verdict, exhaustive anti-alias audit, or proof that rare parameter corners remain stable.

## Actual Seed3 measurements — required, not yet done

Existing measured control, physical Seed3, unconnected audio: `core_hz_rcc=480000000`, `block_frames=48`, `estimated_deadline_cycles=480000`, `blocks=30000`, original 4x RAT composite max callback **370915 cycles**, bypass **3357 cycles**. First-boot serial formatting previously mishandled `%llu` on newlib-nano; use integer max cycle fields, not the garbled ppm label. The bench image in this PR fixes that reporting field.

1. **Do not flash until** the branch's ARM workflow passes, includes the correct named image and reports successful 128KiB BOOT_NONE link. Verify checksums and avoid misattributing an old kit.
2. Flash each image separately through the proven ROM DFU procedure (BOOT held while tapping RESET), leaving BOOT_NONE internal flash unchanged. Check serial banner **after** each flash.
3. Capture all 30,000 callback windows per candidate with the same block size, clock and input conditions. Compare max and ideally distribution/percentiles of callback cycles; absence of missed deadlines is necessary but not sufficient.
4. Use real instrument, strong pulses and upper notes when audio I/O exists; today's unconnected tests are not stressed. Check sound, aliasing, failure modes, clipping, and real ISR/stack headroom.
5. Full graph must fit **<=70% of 480000 cycles (336000 cycles)** under accepted stress conditions, with additional real I/O/ISR margin. Isolated RAT costs must leave room for reverb, delays, modulation and routing. This is a provisional design budget.
6. After tests restore the known-good bypass binary, and **do not merge** optimized modes pending reference listening, correct latency compensation, artifact/source verification, and whole-patch hardware admission.

RAT 97-tap FIR is an *algorithmic approximation*. Its reduced latency matters for dry/wet parallel routes; delay compensation must be explicit. The single-precision clipping model retains double preamp IIRs deliberately: converting IIR state/coefficients without conditioning tests is unsafe.

**Open hardware question:** is the FP32 clipping stage materially cheaper *on Cortex-M7*? x86 host results cannot answer. Do not infer combined savings by simply adding individual percentages.
