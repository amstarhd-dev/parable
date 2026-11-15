# ADR-018 — 1978-family DS-1 and one open feedback capacitor

Status: experimental; founder musical approval and runtime admission pending.
Issue68 selects the family, not authentication of a1978 specimen. Source/version,
component labels and pre-code gates:[Gate0](../SF018_GATE0.md),
[rights](../../third_party/capricorn/provenance.json). No correspondence change.

Choose Meiser/GITEC2019 Figure1 original circuit at9V, not the post1994 dual-opamp
BOM. Q1 is an ideal high-impedance unity buffer with470k load and original10nF C1;
assume1k pickup source plus R1=1k. Its emitter/Q6 source is approximated550Ω.
C2 and bypass switching are omitted. Nominal Q2 Ebers–Moll, R6/R7/R8/R9 and
C3=47nF retain bias, nonlinear asymmetric collector transfer and bass conditioning.
Static transistor table plus the separately linearized C3/C4 feedback network is
a reduced large-signal approximation: dynamic nonlinear C4 current and transistor
charge/storage are not solved. Its exact small-signal feedback is independently
checked. C5=.47uF/R10=100k couples the following input; constant AC100k collector
loading is assumed while preparing the Q2 curve. No measured2SC732 fit.

The sole defect removes C4=250pF parallel to R7=470kΩ. Both configurations have
identical DC bias, transistor table, resistors, preamp, diodes, Tone and Level.
No added EQ or gain fault. Whether this earns a musical place is a listening gate.

Represent TA7136 as an explicit reduced voltage block: original100k linear
three-terminal feedback control, Rupper=100kD, Rlower=100k(1-D)+4.7k and C8=1uF;
frequency-dependent gain`1+Rupper*s*C8/(1+Rlower*s*C8)`,1→22.28 at high frequency.
Assume±3V headroom around4.5V and a20kHz closed-loop pole. Detailed R12=27k
external/internal output loading, R11 sensing input current, C6=150pF/C7=250pF
compensation, finite open-loop gain/slew/noise/supply behavior are omitted. This
is not a substituted DS-1A dual-opamp BOM or calibrated TA7136 macromodel.

Retain R14=2.2k, C9=.47uF, C10=10nF and the nominal antiparallel silicon pair.
Exactly eliminate the *loaded* passive two-branch Tone and Level with capacitor
companions off-thread; runtime evaluates one prepared scalar diode-root curve and
updates four charge histories. Tone uses C11/R15/R17 and R16/C12,20k blend and100k
Level with assumed1M buffer loading. Pot endpoints merge nodes exactly. Output
buffers are ideal; C13/C14 use isolated highpass approximations. Capacitor states
start about the nominal precharged DC point; no power-on transient model.

Reuse main MIT ACME laws and owned Hermite/primitive/FIR patterns; existing
Soundforge PCM/statistics/input, RAT FFT and ngspice runner remain unchanged.
Chow WDF was audited, not imported. No general solver in the callback.

One actual high-note comparison selects8x193tap over4x97tap: worst nonharmonic
spur improves about−39→−50dBr (healthy−43→−56), with a measured cost tradeoff.
Q2 uses first-order ADAA; diode clipping remains in the loaded dynamic network.
Residual aliasing and large-signal reduction error remain, not “alias-free” claims.
No Engine, firmware, storage or live-control integration. Next: founder A/B, then
actual M7 timing and physical/reference-pedal comparison if the voice is retained.
