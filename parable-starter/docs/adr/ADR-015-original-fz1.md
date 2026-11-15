# ADR-015 — original-FZ-1-style three-germanium fuzz

Status: **experimental**, 2026-11-15; founder sound approval pending. SF015/#60.

Use the original3V positive-ground topology pinned in [Gate0](../SF015_GATE0.md),
three finite-beta reciprocal measured-AC128-template GP devices and **one open Q2
470k bias-feed fault**. This is a documented original-FZ-1-style reconstruction,
not a calibrated2N270/vintage specimen. Preserve loaded source, actual Attack
network, both20µF interstage capacitors and3.3nF/56k-shunt/loaded500k output.
The single native kernel owns all states; the faulty case omits only one resistor.

Reuse unchanged main germanium device math/MIT notices and existing owned fixed
passive elimination, FIR, WAV/statistics/excitation and safety guard. A specialized
six-port bounded solve preserves terminal resistances, CBC and interstage history.
Compare1x/2x/4x; exact reference and failed numerical/alias/cost gates stay visible.
No invented gate, noise, random sputter, output-fader fault or bias offset.

Reject leakage-free/memoryless reductions: leakage sets bias and capacitors make
this a dynamic circuit. Reject tuning three transistor leakage parameters merely
to fit an underidentified patent DC example. Reject importing the newer third-party
2N270 card until its separate attribution is cleared. Measured AC128 is defensible
and reusable, but its healthy decay may be less abrupt than the intended voice:
that is a limitation for founder listening, not permission to manufacture gating.

Tradeoffs: exact exponential GP and Newton work may be unsuitable for realtime
Seed3. Constant junction capacitance and fixed temperature omit storage/heating/noise
and specimen spread. Standalone host/ARM evidence does not admit this processor to
Engine. No patch, firmware, storage, living-source or correspondence changes.
Actual guitar audition decides musical acceptance; a separate measured-hardware
cost/fidelity decision would precede any integration. Recording associations other
than independently supported history are tonal references, not provenance claims.
