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

## Selected implementation and observed founder preference

Founder listened to the original Gate1 pair and selected Cloud: “less subtle,
very ethereal.” This selects the core, not approval of every later parameter set
or the damage. Original ethereal files remain committed. The original nominally
compact Cloud control setting actually had a~6.5s T20-extrapolated decay; its
nominal RT60 knob is not the complete diffused network's measured decay.
Retune ONLY original LateLineDecay to.25 for the selected shorter chamber. Keep
all core topology/processing; provide a second Signalsmith envelope retuning and
raw float IR metrics to compare actual decay rather than knob labels.

The common return stage is a stated experimental electrical reduction:
mic -> HP200Hz/LP4500Hz -> fixedx100 preamp + unbiased complementary conduction
(Vbe assumed.7V), referred back to mic units -> gain Rf/Riso -> common soft rail.
Two fixed inversions may restore positive return polarity; only Riso changes.
Choose Rf1k,Riso50k healthy(g=.02), effective partial-short Riso41.667ohm fault(g=24).
The precise resistor values, mic-voltage calibration and soft rail are assumptions,
not a measured studio amplifier. Class-B conduction fact reference:
https://wiki.analog.com/university/courses/engineering_discovery/lab_14 .
The hard knee is continuous but omits junction curvature and thermal/noise physics.

In software, f(v)=.15*tanh(g*sign(v)*max(abs(v)-.007,0)/.15). Both configurations
have exactly this same threshold, filters and rail. Only g changes. The threshold
uses instantaneous filtered mic voltage, not RMS, a note detector or an oscillator.
Its derivative is zero near silence, so a quiet basin exists even when the
large-signal loop can sustain. A linear over-unity loop would fail that requirement.
The resulting return is used64samples later at the SAME reverb's input; microphone,
speaker excitation and applied return are captured. Same room, source, gain,
output calibration3 and mix in each healthy/fault pair. Optional output containment
at.98 is common numerical safety; normal delivered renders reach it zero times.
Return gain changes on the single owner use20ms smoothing; resetting reconstructs
outside audio. This is neither a concurrent interface nor a live preset system.

Initialization audit also found uninitialized seed arithmetic under poisoned
allocation. Initialize affected control/seed/Biquad fields, retain exact patches,
and replace nonportable std::sinf with std::sin(float), preserving float precision.
No core equations, buffers or topology changed. Original-core/selected-core actual
mic WAV equality and poisoned ASan/UBSan are required checks.
