# ADR-021 — borrowed acoustic chamber and return-coupling experiment

Status: Gate1 technical candidate selected, founder sound selection pending.
Full issue75 remains open; no hardware or production admission.

Before editing any upstream processing: rendered complete CloudSeedCore DarkPlate
and complete Signalsmith Basics default/ambient, then compact retunings through
original parameter APIs. Same take03/input peak.15, mono fold,14s files. Compact
candidates have nominal~1.2s decay and120Hz/6.5kHz coloration, useful early return.
Raw retained; separate matched wet copies target.04 RMS. Comparable mixes use
.35dry+.65matched-wet, explicitly recording the engine-specific calibration.
This is a listening comparison, not proof of traditional chamber recognition.

Select the complete Cloud Seed algorithm as the provisional HOST experiment:
MIT core, no framework import, source supports early taps/diffusion, damping and
compact decay without rewriting its algorithm, no non-lock-free atomic helper in
its compiled process. Signalsmith is a strong smaller alternative but its complete
wrapper compiles __atomic_load_8 on M7 (atomic<double> not always lock-free), and
bundled STFX declares BSL outside the existing audited registry import allowlist.
Cloud's67MB fixed state is a major production limitation, not hidden by host RAM;
no Seed3 admission is claimed. Do not silently change buffer capacities now.

Cloud's original source also exposes an invalid uninitialized bool under poisoned
allocator+UBSan. Make only explicit constructor-field initializations before the
selected experiment; keep original pinned files/digests and exact patch evidence.
No topology/equation/delay change. Compare actual output before/after preparation.

The fault will be ONLY electrical microphone-return coupling gain, fed into the
actual SAME core excitation. Any common amplifier threshold/limiting must have a
stated continuous electrical reduction, occur in both configurations and be
measured across weak/strong/zero input. A linear over-unity loop alone does not
satisfy attack-dependent singback. No oscillator, RMS trigger or independent tone
layer is authorized. Stop/report a failed fault hypothesis instead of inventing
another fault. Founder listening, IR benchmark, safety and full issue gates remain
independent of this provisional technical source choice.
