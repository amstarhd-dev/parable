# SF-018 Gate0 — original DS-1 family, one open capacitor

Start2026-11-15 21:30:40UTC; main af159d833bdd0b0e17ccc59fcf7edfa506ed98f5.
Issue68/AGENTS/Constitution/main README/status/ADR001 and Soundforge recipe read.
Root AGENTS absent; project AGENTS present. Existing utility/registry baseline3/3
passes before code. Issue68 requires focused new tests and modified dependencies,
not complete older circuit suites. No open PR/Scorpio/65 code imported.

Pin Meiser/GITEC2019 Figure1 (PDF SHA in provenance), original1978 Si Q2 + TA7136
family,9V. C1=10nF, C2=.47uF,C3=47nF,C4=250pF,C5=.47uF,C6=150pF,C7=250pF,
C8=1uF,C9=.47uF,C10=10nF,C11=22nF,C12=100nF,C13=47nF,C14=1uF.
Q2 R6=100k,R7=470k collector-base,R8=10k,R9=22. Single fault C4 absent;
DC bias and all other parts identical. Buffer/Q6 effective source550Ω per article;
input Q1 ideal unity,470k input load, assumed pickup source1k plus R1=1k.
Q2 nominal main MIT ACME Ebers-Moll Is1e-14,beta350,reverse-beta4,300K; not a
measured2SC732. Prepared static BJT curve and exact linearized C3/C4 network;
large-signal nonlinear capacitor feedback/transistor charge omitted explicitly.
C5 coupling into R10=100k; its AC loading approximated constant in Q2 curve.

Reduced TA7136: original100k linear three-terminal Distortion feedback divider,
R13=4.7k/C8=1uF, gain1+Rupper/(Rlower+R13); assumed finite20kHz closed-loop pole,
±3V headroom around nominal4.5V. Not a dual-opamp model, not measured TA internals;
R11 ideal-input feedback sensing, R12/C6/C7 detailed loading/compensation omitted.
Actual R14=2.2k,C9,C10 and silicon pair retained with loaded passive Tone/Level.
Tone: R16=6.8k/C12 dark; C11/R15=2.2k/R17=6.8k bright;20k Tone,100k Level,
assumed1M output-buffer load. Exact endpoint merging; no fake tiny pot resistors.
Output buffers ideal; C13/C14 isolated highpass approximation. No bypass/LED etc.

Borrow main PCM/analysis/input/guards, ACME physical laws, owned companion/filter/
Hermite/FIR patterns. Chow WDF diode pair inspected; no import or new framework.
Predeclare compact gates: Q2 DC within5mV, static curve20mV, linearized feedback
AC magnitude within3%, loaded tone within2%, nominal diode curve within1mV and
polarity symmetry. One high-note alias observation (report actual), finite/control/
reset/silence/determinism and callback restrictions.4x97tap initial quality;
change only if actual alias evidence warrants it.6–10 credited take03 WAVs,
low/high Distortion, same source/Level; report C4-open difference honestly even
if negligible. Founder musical acceptance and physical Daisy remain unproven.
