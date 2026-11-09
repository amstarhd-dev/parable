# ADR-003 — LabRAT Circuit Model and Fault Architecture

Status: **PROPOSED — independent technical review and founder audition pending**.
PAR-007 supersedes the prior proposal to integrate PAR-006. Neither is in patch v1.

## Selected foundation

Port the circuit-derived feedback transfer and pot law from PiRAT/Rodent, retain
BSD-2-Clause and MIT notices, and extend the missing component relationships.
Audit: [LABRAT_SOURCE_AUDIT.md](../LABRAT_SOURCE_AUDIT.md). PiRAT is not adopted
wholesale: its approximate 5 kHz input filter, fixed diode spline, ideal op-amp
and independent post-filter cannot honestly represent all requested faults.
The generic PAR-006 drive is not used. Its owned WAV helper is reused from the
unmerged PR #11; this branch starts on main after PR #9, without merging #11.

Reference is the classic **LM308 / silicon-diode RAT topology, 100 kΩ distortion-
pot variant**. The core values are corroborated by Rodent/PiRAT and BYOD's published
RAT schematic. This is a documented nominal reconstruction, not a serial-number-
verified 1978/1979 factory unit. ElectroSmash access is blocked (proxy HTTP 403).
Historical year/BOM authentication remains an explicit gap, not a fidelity claim.

## Circuit and port

Input: 22 nF coupling, 1 MΩ bias return, 1 kΩ series, 1 nF RF shunt. Feedback:
560 Ω/4.7 µF and 47 Ω/2.2 µF series branches, 100 kΩ distortion pot shunted by
100 pF. The borrowed ideal transfer is G(s)=1+Zf(s)(Y1(s)+Y2(s)). A finite open-loop
A(s)=A0/(1+s/wp) gives Gactual=A G/(A+G), discretized by a fixed-array binomial expansion of the standard bilinear
substitution with double history. AudioLib has no located source license; its
implementation is not imported. This preserves frequency-dependent gain;
it is not an arbitrary pre-gain followed by a waveshaper.

A0=200,000 and GBW=1 MHz, 0.3 V/µs slew and ±4 V relative-to-bias output swing at
9 V are **reduced-model assumptions**, not independently verified 9 V LM308
measurements. External compensation is nominally 30 pF; full transistor dynamics,
current limiting, common-mode behavior and saturated-feedback recovery are not
modeled. Rail/slew limits follow the linear closed-loop model, as in upstream's
rail-limited staging; this is an explicit approximation under hard saturation.

Post-amplifier: 4.7 µF coupling -> 1 kΩ -> antiparallel silicon diodes ->
(1.5 kΩ + 100 kΩ Filter pot) -> 3.3 nF shunt -> 22 nF coupling into 1 MΩ buffer
input. These passive elements and diodes are solved together using trapezoidal
capacitor companions and a control-prepared inverse nonlinear-port table.
Diodes use Is=2.52 nA, n=1.752, Vt=25.85 mV; comparisons with upstream's published
silicon spline and an independent SPICE exponential diode are required. Diode
capacitance/series resistance and individual device tolerances are omitted.

The output JFET is a high-input-impedance unity buffer approximation, then 10 µF
coupling, 10 kΩ series, 100 kΩ Volume pot and 1 MΩ load. JFET coloration/operating
point are not validated. Interface: 1 input unit = 1 V model excitation; output
volts are divided by 4 for numerical headroom. This is not measured Daisy analog
calibration. Final ±1 rails protect numerical output, not an analog loudness claim.

4x FIR interpolation and decimation surround the *whole* nonlinear circuit;
coefficients and lookup preparation occur with processing stopped. Fixed capacity,
bounded processing, finite/denormal handling and deterministic reset are required.
No live-safe component automation or patch activation is claimed.

## Three causal variations

- Positive diode disconnected: remove its current branch, rather than substituting
  a biased clipping curve. Coupling capacitors can charge/rectify; measure the raw
  clip-node offset and output after AC coupling, including transient behavior.
- Ruetz: disconnect the 47 Ω/2.2 µF feedback leg. Explicit open topology, not a huge
  resistor approximation. This is an intentional modification, not a random fault.
- Feedback compensation capacitor 100 pF -> 100 nF: a 1000x wrong-part experiment (e.g. 100 nF fitted for 100 pF), changes LM308 closed-loop gain/
  bandwidth through Zf. This is the *feedback* compensation capacitor, not the
  separate 30 pF pins-1/8 compensation capacitor. Its effect is validated against
  circuit AC simulation; no unsupported supply-sag effect is invented.

## Alternatives and acceptance

BYOD Mouse Drive supplies editable WDF components, but is GPL-3.0 with a larger
JUCE/chowdsp context and covers the clipping stage rather than the whole RAT path.
It is audited, not copied. Directly shipping PiRAT would save work but preserve
its unsupported approximations and fixed clipping topology. Full real-time SPICE
is inappropriate here; offline SPICE supplies independent reduced-circuit evidence.

Founder must judge the healthy low/medium/high, Filter/dynamics and three modified
renders, then accept/revise/reject. Circuit-response agreement supports only the
modeled components and assumptions; it does not certify a historical pedal or
musical identity. No factory pedal recording/hardware measurement is present.
Permanent patch/descriptor integration follows review with explicit migration.
Astrology, existing patch v1, native Engine/preparation and hardware controls stay
unchanged. Project-wide licensing remains an owner decision; borrowed notices do
not license the rest of Parable.
