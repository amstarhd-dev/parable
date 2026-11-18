# Codex Assignment — SF-025

## Jupiter in Virgo: Tiled Room

### A Lexicon PCM70-inspired Rich Chamber and its pathological failure

**Repository:** `amstarhd-dev/parable`\
**Status:** Experimental standalone DSP\
**Disposition:** New branch, independent PR, leave unmerged.

### 1. Artistic brief

Jupiter signifies expansion, abundance, grandeur, and spaciousness. In Virgo, Jupiter is in detriment: confined to a small, meticulously organized, utilitarian environment.



**Jupiter wants a cathedral. Virgo gives him a bathroom.**



The result should not be a weak or merely shortened hall reverb. It should be a remarkably vivid, dense, bright, reflective artificial room.



Our principal reference is the celebrated **Tiled Room preset of the Lexicon PCM70 V2**, built around its Rich Chamber algorithm.



We want the acoustic personality that made this small digital room a classic, not merely a conventional bathroom simulation.



The healthy sound should be:

- Compact but surprisingly expansive in perceived reflection density.
- Bright, immediate, and highly articulate.
- Hard-surfaced and reflective, with tightly controlled decay.
- Dense rather than obviously discrete or slapback-like.
- Capable of making guitar transients sound larger without generating a long ambient wash.
- Distinct from our existing plate, chamber, coupled rooms, and grand hall voices.

### 2. Prior-art investigation — mandatory

Before implementing new DSP, investigate:



**Primary reference:**



Sean Costello, Valhalla DSP, *Eos Tips and Tricks: Recreating the PCM70 Tiled Room* (2009).



[https://valhalladsp.com/2009/12/22/eos-tips-and-tricks-tiled-room/](https://valhalladsp.com/2009/12/22/eos-tips-and-tricks-tiled-room/)



**Original manufacturer documentation:**



Lexicon PCM70 Owner's Manual, particularly the Rich Chamber family and Tiled Room program.



Investigate the published algorithm descriptions, preset parameters, and available authorized listening demonstrations.



Costello's reconstruction provides useful parameter clues, including a short predelay, compact size, frequency-dependent decay, and absence of modulation. These are not a complete DSP specification.



Research at least two technically credible implementations or algorithm families capable of approximating the reference.



Prefer existing permissively licensed code. Determine whether any of Parable's existing reverberation engines can be reused without compromising the intended sound.



Do not copy proprietary Lexicon firmware, proprietary DSP code, or third-party commercial plugin implementations.



A properly attributed independent recreation of the acoustic character is acceptable.

### 3. Healthy voice — reproduce the character first

Build a compact, efficient Rich Chamber-style reverb.



Prioritize:

- Very short predelay.
- High early-reflection density.
- Rapid diffusion.
- Bright but controlled spectral decay.
- Strong transient response.
- Short-to-moderate reverberant tail.
- No gratuitous modulation, shimmer, or chorusing.
- Mono compatibility, with stereo operation if the selected architecture supports it efficiently.

Do not assume that a literal tiled bathroom impulse response will reproduce the PCM70 sound. The reference is an engineered digital algorithm, not necessarily a physically accurate room.



Compare the candidate against the documented PCM70 characteristics and any legitimately accessible reference recordings.



**First milestone: deliver the healthy sound for listening before developing the fault.**



If the result is merely an ordinary short reverb, continue improving the healthy implementation or document why the selected algorithm cannot achieve the target.



Do not manufacture a claim of Lexicon authenticity from matching preset numbers alone.

### 4. Retrograde failure — ceramic resonance

The healthy Tiled Room represents Jupiter's detriment. Retrogradation introduces an additional mechanical or algorithmic failure.



Investigate a single plausible alteration to the selected reverberation structure that creates abnormal resonant accumulation.



Preferred concept: a reflection or damping path becomes insufficiently attenuated, allowing particular frequencies to circulate excessively within the small, rigidly reflective space.



The result should resemble **a bathroom that begins to sing**.



Desired musical characteristics:

- Noticeably metallic or ceramic tonal coloration.
- Frequency-selective ringing.
- Stronger excitation from suitable guitar notes and transients.
- Audible persistence after attacks, with bounded decay.
- A distinct difference from Leo's grand-hall acoustic focusing.

Do not simply add a sine oscillator, resonator plugin, or metallic sound sample to the output.



The fault must arise from a defensible alteration of the same reverberation network.



Choose exactly one fault mechanism and retain identical controls and source excitation between healthy and damaged states.



If the proposed fault becomes indistinguishable from Leo's failure, investigate a different credible mechanism within the same architecture and document the choice before implementation.

### 5. Listening requirements

Use the established licensed guitar recording.



Deliver:

1. Healthy Tiled Room — same-gain recording.
2. Damaged Tiled Room — same-gain recording.
3. Healthy and damaged loudness-matched comparison.
4. Healthy wet-only return.
5. Damaged wet-only return.
6. Healthy and damaged impulse responses.
7. Short transient diagnostic highlighting reflection density and ringing.
8. A comparison with an existing Parable room or hall, adjusted to a comparable decay where practical.

Provide actual playable WAV files and record source identities, gain settings, and processing parameters.



Keep the dry guitar present in the principal mixed auditions. Wet-only files are separate diagnostic recordings.



Do not impose a universal dry/wet policy on the processor itself.

### 6. DSP and hardware constraints

This is another candidate voice for the Daisy Seed, not permission to build an unlimited host-only reverb.

- Favor bounded, fixed-memory processing.
- Avoid callback allocations, locks, file I/O, and logging.
- Support the established sample rates.
- Measure persistent state, flash footprint, processing cost, and stack requirements.
- Distinguish actual ARM evidence from host measurements.
- Preserve existing implementation and canonical interface contracts.
- Do not modify previous Jupiter voices.
- Do not merge outstanding PRs.
- Do not create a new general-purpose reverb framework.
- Keep testing focused and proportional.

The healthy algorithm must be efficient enough to justify continued consideration for embedded deployment.



If the selected implementation is too expensive, report that early instead of completing an unusable model.

### 7. Acceptance criteria

**Musical acceptance is primary.**



The experiment succeeds only if the healthy voice convincingly exhibits the intended compact, brilliant, dense PCM70-style character and the fault adds a distinct, causally defensible musical personality.



Technical acceptance additionally requires reproducible rendering, clear source provenance, bounded processing, focused tests, and honest hardware-resource reporting.



Do not claim subjective sonic equivalence to an original Lexicon without a properly controlled comparison.



Do not spend hours on redundant validation while the listening character remains unresolved.

### 8. Deliverables

Open one PR containing the implementation, source/license documentation, listening recordings, focused tests, resource measurements, and a concise comparison with the PCM70 reference.



Report explicitly what was borrowed, what was independently implemented, and what remains unverified.



Leave the PR unmerged for founder listening and independent review.



**Priority: Recognizable Tiled Room character → fault quality → Daisy feasibility → focused verification.**



The objective is not a scientifically perfect bathroom. It is a compelling, historically informed digital reverb voice with an unmistakable identity.