# SF-019 — Jupiter Taurus: experimental plate-character return

**Refs:** Issue #71; frozen Parable Lexicon Jupiter/Taurus = plate reverb, damaged plate suspension/damping. **Status:** playable-first host prototype, no founder approval or physical M7 admission.

## Borrow-first decision

- Core is a compact, independent native C++ adaptation of Jon Dattorro's 1997 figure-eight topology. Inspected MIT implementations: [johnhw/dattoro_reverb](https://github.com/johnhw/dattoro_reverb) at b69fb9eb505a1bb4d31158674e4b63da38fad871, and [mjarmy/dsp-lib PlateReverb](https://github.com/mjarmy/dsp-lib/blob/main/PlateReverb.hpp) at a26e67c8c9a12038cc51fdec5f7bed3941ce465b. Pinned source, notices and rejected/copyleft candidates: [provenance](../third_party/plate/provenance.json).
- This is an **algorithmic plate-character reverb**, NOT a finite-difference steel sheet, actual EMT 140 suspension, electromagnetic pickups or calibrated plate. Dattorro delay controls are not mechanically defined suspension parameters.
- Two interlocked delay/all-pass/damping branches use Dattorro's nominal 29,761 Hz ratios; 14 output taps are folded to mono (potential phase cancellations not yet auditioned); two interpolated low-speed modulated all-passes reduce static metallic delay resonances.
- Core allocates delay buffers in stopped \`prepare()\`, not in \`process()\`; no target-specific SDRAM owner or Engine/pool integration. No callback trig/libm beyond basic finite classification; scalar oscillator is an inexpensive reduced model. Heap use during preparation is unsuitable for a realtime callback.

## Mechanically motivated but deliberately reduced fault

One variable changes: **mount contact clearance**, a dimensionless displacement 3.0 healthy -> 0.045 damaged; all mechanical and reverb coefficients stay identical.

A driven local mode at nominal 680 Hz, damping ratio 0.09, obeys a stable per-sample velocity/displacement update:

\`v += dt*(omega^2*(2.5*wetBody-x) - 2*zeta*omega*v)\`
\`x += dt*v\`

If motion exceeds the unilateral contact gap while traveling outward, velocity reflects with restitution 0.19; the nominal kinetic energy just across an impact drops by factor \`0.19^2\`, absent continued external forcing. Brief sticking and a 1.5% interior release margin stop numerically repeated impact events while resting against the boundary. The change in velocity supplies a bounded next-sample mechanical return (clipped to +/-0.12) into the **tank input diffusion**, not an added noise/sample or unrelated mixed buzz stream.

**Physical caveats:** driving the contact mode from an algorithmic wet signal is *phenomenological coupling*, not derivation from a two-dimensional plate displacement field. Contact return is phenomenological; the full composite does not carry a proven passivity theorem. The test has credible impact causality but does not prove a genuine plate suspension failure sound. The setting is an experimental simulation parameter, not measured millimeters.

## Measured host evidence (2026-11-16)

Locally compiled standalone core/tests on GNU C++ 14.2 with C++17 \`-O0\` and \`-O2 -DNDEBUG\`; test passes both configurations (preliminary smoke tests only; no ASan, ASan/UBSan, registered CTest or ARM compiler run yet).

At 48kHz, 2 s controlled 220Hz input followed by decay, same wet setting and damage clearance only:

| Input sinusoid amplitude | Healthy collisions | Damaged collisions | RMS healthy-vs-damaged difference |
|---:|---:|---:|---:|
| 0.0001 | 0 | 0 | 0 |
| 0.04 | 0 | 514 | 0.001116 |
| 0.30 | 0 | 1,679 | 0.003814 |

The one-parameter contact has level dependence; numerical safety/finite checks, 44.1/48/96k and reset/stop/invalid prepare were checked. No physical contact force/tank calibration or independent SPICE/physical reference test has been performed.

48 kHz allowed first-eight-seconds Guitar-TECHS take03 audition, 2s tail, software input scale0.8, dry/wet 0.66/0.34 (no offline scale in DSP). Guitar material is credited separately in [audio notice](../third_party/plate/AUDIO_NOTICE.md). Preview ZIP is delivered in the conversation; not tracked as WAV files in GitHub.

| File | RMS | Peak | Offline gain |
|---|---:|---:|---:|
| dry raw | 0.04110 | 0.21252 | 1 |
| healthy raw | 0.03299 | 0.15509 | 1 |
| damaged raw | 0.03280 | 0.15440 | 1 |
| healthy matched | 0.06000 | 0.28211 | 1.81894 |
| damaged matched | 0.06000 | 0.28241 | 1.82908 |

Guitar preview damaged vs healthy raw RMS difference 0.001672, approximately 5.1% of healthy RMS. **A calculated difference does not mean it is musically obvious.** Native host \`-O2\` wall timing includes full \`process()\` on 480k frames: healthy 0.1244us/frame, fault 0.1300us/frame, clock not controlled. **This is not a Seed3 execution estimate or deadline proof.**

Prepared delay-buffer heap capacity observed at 48k: 146,600 bytes. Does **not** include vector object/state/allocator overhead or a four-preset pool. No target-SDRAM placement or target stack/CPU evidence. No hardware audio, boot, noise, tail-switching, permissions, performance/alias proof or founder musical approval. The existing mono native patch schema remains limited to utility/RAT effects, so this specimen is not loadable as a saved patch.

## Reproduction / immediate review

With the new source branch, compile a focused test (no unrelated pre-existing DSP suites):

\`\`\`sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iparable-starter/include \
  parable-starter/src/parable_plate.cpp parable-starter/tests/test_plate.cpp -o /tmp/plate_test
/tmp/plate_test
cmake -S parable-starter -B /tmp/plate-build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/plate-build --target parable_plate_test parable_plate_lab -j2
ctest --test-dir /tmp/plate-build -R plate_circuit_and_contact --output-on-failure
/tmp/plate-build/parable_plate_lab /path/to/prepared-take03-20s-mono16.wav /tmp/new-empty-output-dir
\`\`\`

Expected take03 prepared SHA256: \`75ae5e73c72e6774fb75e6318dc91ba66347fb65290949e947144033eb648f52\`. The laboratory command demands an absent output directory, and produces the common dry, raw healthy/fault and two separately matched WAVs; only the first 8 seconds of input are used, with 2 seconds of tail.

### Unmet gates and next decision

The stop condition is not fully cleared: **a healthy/fault guitar preview and input-dependent impact have been demonstrated, but neither human listening nor a physical reference validates the metallic suspension-buzz character**. Founder should audition the raw and matched pair. If the effect is musically weak, refine/reject this *specific coupled mount model* rather than inventing a second fault. Separately perform ASan/UBSan, callback guard, spectral/alias and impulse-tail measurements, actual pinned ARM object audit and resource admission **before declaring the full issue accepted**.

All existing Engine, patch, firmware, storage, living-source, correspondence and prior Soundforge voice sources remain untouched. Leave unmerged.
