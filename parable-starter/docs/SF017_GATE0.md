# SF-017 Gate0 — proportional playable-first slice

Main af159d833bdd0b0e17ccc59fcf7edfa506ed98f5;61/63/65 remain open, not imported.
Root AGENTS absent; project AGENTS, Constitution, README/status and Soundforge
recipe read. Issue66 overrides the old blanket repeat-baseline instruction:
verified unchanged main already passed38/38 Debug and Release here underGNU14.2.0
(CMake3.31.6), recorded before any #65 implementation. No #65 code/profile borrowed.
One full final Debug/Release -j2, targeted new sanitizer and ARM object audit only.
Start of work2026-11-15 20:20:39UTC. First WAV time will be recorded honestly.

Aion Skywave1.1.0/2024-01-21 p7 selected,9V input/core, gain switch center-off,
trim0ohms, C1 omitted/C2=1uF. Q1 2N5088 NPN intoQ2 2N404A PNP; exact DC R11/R12/R13
supply filtering/divider andR1/R2/R3/R6/R7 included. Single faultR3(Q1 collector
fromVC)5.6k->10k. Independent ngspice44.2 confirms Q2 collector~2.81->4.79V;
no other component changes. Artwork/code not copied. Manufacturer description
supports desired family, not calibration. Triad TY-141P datasheet2019-05-31:
1:1, windingDCR820/1070ohms,10k matching impedances (NOT winding resistances),
200–15kHz. Inductance, B-H, hysteresis and saturation onset are unspecified.

Reduction: stopped5-node Ebers-Moll preamp curve including finite source/base
loading and gain feedback. Quasi-static nonlinear curve plus exact small-signal
capacitor/feedback filter (C2/C3/C7/C8/C9). C4/C6 are guitar-band AC shorts, C5 is
disconnected; ideal AC decoupling of supply. Large-signal dynamic transistor charge
and changing output impedance are omitted. Measured AC128 Gummel–Poon constants
are inappropriate for2N404A; nominal Si/Ge EM law reused from MIT ACME foundation
via existing Muff/wavefolder, with beta400/100,Is1e-14/1e-9 and50uA assumed Ge
collector-emitter leakage. No measured transistor fit.

Transformer: passive ideal1:1 with actual windingDCR, finite R9/R10/Level/load,
C10 charge and flux states. Assumed L0=20H, flux scale8mV*s, i(phi)=phi/L0+
phi^3/(L0*scale^2). Positive stored energy, nonlinear differential inductance;
no remanence/hysteresis claim or measured TY-141P curve. Saturation is caused by
actual integrated winding voltage and magnetizing loading, not output tanh.
Fixed3 scalar Newton updates per internal sample, not a per-sample BJT solver.
Prepared513-knot Hermite+primitive,4x FIR+ADAA; evaluate1x diagnostic for alias.
State ceiling12KiB; aim~2000 M7cycles/frame, unmeasured design budget only.

Borrow audit: ACME.jl3832ccafb952ee2ca0a27286c091d613062c23fd MIT current/transport
foundation already in main; owned main wavefolder FIR/curve technique, lab_wav,
analysis and input receipts reused. Paulllux/fetcomp-dsp de18f5ac793e36397c725abdca7fcb8c08760ce2
FetLimiterDsp.h inspected (MIT): JUCE-dependent fitted Jiles–Atherton compressor,
not this preamp, no import. GPL ComplexNonlinearities audit-only/no import.

Targeted gates before implementation: DC nodes within5mV of independent SPICE;
prepared static points within20mV; small-signal frequency magnitude within5% of
explicit reduced reference at100/1000/5000Hz; one short transient within5%+1mV
against equivalent reduced network; nonlinear-vs-linear magnetics must differ
and preserve energy/loading/LF state; high-note residual must improve>=6dB at4x
versus1x diagnostic;44100/48000/96000 guard/invalid/reset/silence stress. Retain
failed gates and report partial/blocker if hypothesis fails, no expanded campaign.
Ten8s+tail take03 dry/raw/matched WAVs, same Level and input calibration.
