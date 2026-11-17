# Codex Assignment — SF-022
## Jupiter in Leo: Grand Hall Reverb

**Repository:** `amstarhd-dev/parable`  
**Status:** New experimental voice. Do not merge.

### Objective

Build a professional-quality **Grand Hall reverb** with one physically motivated failure mode.

Healthy Leo should sound magnificent: an immense, reflective concert hall, with strong early reflections, a spacious reverberant field, and a sustained, luminous decay.

This is not shimmer, chorus, or an artificially inflated wash. It is an acoustic space with scale, depth, and recognizable architecture.

### 1. Borrow before building

Survey established, legally reusable hall-reverb DSP, including:

- Freeverb3 and its relevant hall algorithms.
- Dattorro-style tank implementations.
- Signalsmith Stretch / reverb facilities, where appropriate.
- Other mature, efficient FDN or allpass-based hall implementations.

Do not restrict the investigation to these examples.

**Run at least two credible candidates on identical guitar material before choosing.** Compare sound quality, license compatibility, RAM, computational demands, and integration complexity.

Preserve a complete mature algorithm wherever practical. Do not create another custom reverb engine merely because implementing one is convenient.

**Hard constraint:** Daisy Seed feasibility is mandatory. Reject any candidate whose unavoidable memory requirements exceed the available deployment architecture. Report actual memory allocations, not just compiled code size.

### 2. Healthy voice

The hall should have three perceptually distinct components:

1. **Stage:** Direct sound remains articulate and relatively close.
2. **Architecture:** Early reflections communicate the dimensions and geometry of a large performance hall.
3. **Audience space:** A dense, diffuse late reverberation develops behind the instrument.

Suggested listening target: a luxurious orchestral hall with a roughly 2.5–5-second configurable decay, smooth high-frequency damping, and excellent stereo spaciousness where supported.

Support mono input/output without making the hall collapse perceptually.

The healthy state must remain recognizably the original, proven borrowed algorithm.

### 3. Failure mode — Acoustic focusing

Leo's failure is **the hall begins to concentrate its own reflections**.

Model a defective acoustic focusing condition: excessive coherent energy accumulates around particular reflection paths or resonant bands, producing a conspicuous, frequency-selective bloom.

It should sound as though the architecture itself has begun to sing along with the guitar.

The failure must arise from a plausible change to the same acoustic/reverberation system, such as altered scattering, reflection coherence, or resonant feedback—not a separately mixed-in synthesizer or arbitrary oscillator.

Explore one fault mechanism, not a collection of unrelated effects.

Desired behavior:

- Weak playing: spacious, largely normal hall.
- Strong notes or chords: selected resonances accumulate and bloom.
- Muting: the excited space decays naturally.
- Reset: energy clears without stuck ringing or discontinuities.

A healthy hall should sound expensive. The damaged hall should sound **impossibly grand and slightly dangerous**.

### 4. Implementation constraints

- Preserve Parable's existing effect interfaces and test infrastructure.
- Keep healthy and damaged states within the same selected reverb architecture.
- No heap allocation, filesystem I/O, locks, or logging in the audio callback.
- Test 44.1, 48, and 96 kHz.
- Measure RAM, flash, stack, and processor cost, distinguishing host measurements from actual ARM evidence.
- Avoid extensive unrelated refactoring.
- Do not modify the outstanding Cancer chamber PR.
- Do not claim hardware suitability from a successful cross-compile alone.

**If no candidate can meet the hardware budget, stop and report the blocker rather than building an unusable effect.**

### 5. Audition deliverables

Use the same established guitar sample for comparability.

Provide:

- Healthy hall, original implementation.
- Healthy hall, adopted implementation.
- Damaged hall, identical input and gain.
- Wet-only healthy and damaged recordings.
- Short transient, sustained chord, silence, and reset tests.
- At least one matched-loudness comparison.

Record actual processed audio, not synthetic demonstrations standing in for the guitar signal.

### 6. Acceptance criteria

The PR must demonstrate:

1. A legitimate, traceable source implementation with compatible licensing.
2. A meaningful hall-reverb quality advantage over Parable's previous generic ambience.
3. An audible and causally defensible acoustic-focusing failure.
4. No unexpected instability, numerical divergence, or unbounded feedback.
5. Credible evidence of deployability within the Daisy Seed's available resources.
6. Reproducible audio and focused validation results.

Do not spend hours proving inconsequential numerical invariants while the actual sound and hardware feasibility remain unproven.

**Priority order: musical quality → hardware feasibility → fault character → focused validation.**

Open one PR with the recordings, evidence, source attribution, and any unresolved limitations. Leave it unmerged for independent review.