# SF-011 Gate 0 — original-style Centaur / one clean wiper contact

2026-11-13 UTC, before native edits. Main **dbde3438b41ad834ba7801cc111be1c16adfbdad**
includes the independently merged #52 and #50. The issue's #52-open note is now
historical. #42/#48 remain open and untouched at heads057312b/9f8548c. No other PR
was changed, cherry-picked or merged by this task. Read project AGENTS,
Constitution, README/status/kickoff, Soundforge recipe/baseline, relevant ADR001/
PAR008/PAR011 budgets and actual RAT/CMOS/Ge/Muff/TS808 models/reports/tools.
Root AGENTS and frozen Lexicon are absent; project rules exist under parable-starter.

Clean GNU14.2/CMake3.31.6/Python3.12 baseline: **34/34 Debug955.50s,
34/34 Release187.41s**, before repository edits. Exact commands from root:

```sh
cmake -S parable-starter -B /workspace/work/sf011-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/sf011-Debug -j2
ctest --test-dir /workspace/work/sf011-Debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /workspace/work/sf011-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/sf011-Release -j2
ctest --test-dir /workspace/work/sf011-Release --output-on-failure --no-tests=error -j2
```

Capture270 preexisting source/header/firmware/fixture hashes and27 fresh
canonical/living audio/report hashes. Existing goldens must not be refreshed.

## One original drawing and numbering

Select **Martin Chittum,15 April2009, original Centaur unit S698** trace, hosted
by Aion: https://aionfx.com/app/files/schematics/klon-centaur-schematic.png
Retrieved138182bytes, SHA256
`20f736382cd5ccb74d143d519cc9d39135f153ef8422a18d7be175272e83eb39`.
Only component facts transcribed; no artwork/text/layout/code copied or claim of
license to redistribute that drawing. This is not a clone/KTR or a measured unit
in our possession. Target includes its R1=10k, R11=15k, R28=100k and original
R21=1.8k/R23=4.7k, not the later debated tone-resistor swap.

Independent ElectroSmash full drawing: mirror commit
`ebf6b8604d663c1832d06682e74940830cdc5490`,
`images/klon-centaur-analysis/Klon-Centaur-Schematic-Big.png`, Git blob
`031457771c4f06fcb2b15ae9af2b811124323bfe`, SHA256
`69e2ec4527bcb7a7a14d61d4be00b08dc5919e90ea6731b52cd0addafb7ec673`.
Its labels differ: **RV-GAIN_b = Chittum P1A(clean)**,
**RV-GAIN_a = P1B(drive)**. Chittum R241 is the100k tone feedback resistor;
R26/R27 are68k output/bypass bleed parts. Do not blindly copy article label typos,
its nominal27V prose or a different MAX1044 BOM into the selected ICL7660S trace.
Aion's original-trace discussion and independent MNA analysis corroborate topology.
Original ElectroSmash endpoint returned502; pinned mirror is accessible.

| Selected components | Value / connection |
|---|---|
|R1/C1/R2|10k /100n /1M bias to Vref; external1k source|
|U1A|TL072CP input follower,0..9V supply|
|C2/R3/R4|4.7u /100k ground /560Ω bypass feed; retain effect-on loading|
|C3/R5/C4|100n buffer→pre;5.1k parallel68n pre→clean-upper|
|R6/C5|10k parallel68n pre→U1B plus|
|R7/C16/R19|1.5k pre→FF1;1u ground;15k FF1→summing minus|
|R8/C6/R9|1.5k clean-upper→Vref;390n+1k series to Vref|
|P1A/P1B|oppositely effective100k linear gangs; common knob g∈[0,1]|
|R10/R11/C7|2k from P1B lower;15k parallel82n to U1B minus|
|R12/C8|422k parallel390p U1B feedback|
|C9/R13/D2/D3|1u+1k series; nominal germanium antiparallel shunts to ground|
|C10/R16|1u clip→postclip;47k postclip→summing minus|
|C11/R15|2.2n+22k postclip→clean-wiper|
|R17/C12/R18|27k in parallel with **27n+12k series**, sum-minus→clean-wiper|
|R20/C13|392k parallel820p feedback around U2A|
|R21/P2/R23|1.8k+10k linear+4.7k between sum-out and tone-out|
|R22/R241/C14|100k input /100k feedback;3.9n tone wiper→U2B minus|
|C15/R25/P3|4.7u+560Ω;10k linear Output to ground|
|R26/R27/R28|68k bypass→output /68k level-top→output /100k output→ground|
|R29/R30/C18|27k divider from9V/ground,47u at actual Vref|
|External load|1M at jack, in addition to the selected output network|

Retain all16 signal caps plus physical passive Vref divider/cap:17 histories.
Effect-on switch connections fixed; no bypass UI. Both clean routes and the
bypass-bleed/output loading remain, including their mutual nonlinear loading.
P1B upper=100k*g to Vref, lower=100k*(1−g)+R10. P1A top=100k*g,
bottom=100k*(1−g), wiper separately contacts the loaded clean net. Exact pot
endpoints; no invented tiny resistor replacing a mathematical wire.

## One physical contact, chosen before implementation

**Fault: P1A wiper-to-external-wiper-lug contact, track node→clean-wiper net**
(the external net joins R15, R17 and R18). Nominal ideal0Ω; fault **100kΩ series**.
Track end resistances still follow the same knob; P1B stays healthy. This models
one resistive/near-lost contact, not a broken track, second gang or user severity.
No added gain, noise, gate or output mixer. Contact resistance changes the actual
node conductance/load/poles/phase. Nominal/increased10k/100k/1M/near-open1G were
probed in independent SPICE first; only100k is selected for the native fault.

At~1002.37Hz, small-signal output magnitude healthy→100k contact:
g=0 .73637→.21443 V/V, g=.5 6.98504→7.45856, g=1 88.89822→115.39267.
Low-gain phase25.51°→−78.29°, high-gain−19.73°→3.47°. Thus it is not just
volume reduction or complete clean removal. Atg=.5/.15V/1k,40ms full nonlinear
probe: healthy jack−.6580..+.6400V;100k fault−.6918..+.6750V. Physical Vref
ripple is~.1mV here. No musical adjective or pleasantness is established.

25 independent linearized DC cases verify quiescent4.5V/diode0V. Early behavioral
SPICE OP attempts falsely accepted pseudo-transient bias points; rejected using
DC/KCL checks. For DC/AC, use explicit linear G/R/C amplifier law, rigorously the
same law inside quiescent slew/rail bounds. Full nonlinear transients start from
that verified equilibrium using capacitor IC/UIC; no held bias during playback.
Scaling internal macro capacitor and current together to1n avoids large artificial
currents without changing the ODE. Full-span, bias, KCL and refinement gates are
mandatory; simulator return code alone is insufficient.

## Reuse, supplies, bounded implementation and accuracy

Audited ChowCentaur `f3bb633a593b6fbb22a44c1ef9d1dbedbfe92d5b`, root BSD-3-Clause
license plus gain/WDF/diode/feedforward/summing files. It is credible prior art,
but block decomposition/ideal terminations/independent clean paths and endpoint
resistor floors do not directly preserve this contact's full mutual loading.
Its BSD-3 code is outside the unchanged MIT/BSD-2 import policy; audit-only, not
an assertion that its license is nonpermissive. No JUCE/WDF framework or RNN import.
Reuse MIT ACME.jl3832ccaf diode/finite-pole foundation/full notice; main's exact
passive elimination, capacitor companions, pivot/damping, FIR, PCM83/stats58/
excitation31/guard87 lines and unchanged20-line SPICE wrapper. No generic solver.

Nominal diode IS=600nA,n=1.5,300K, eg=.67eV, no intrinsic charge/series resistance.
This gives~.35V at5mA, consistent with a published original-unit observation, but
is a generic reduced pair, not a measured vintage manufacturer model. Pin the
independent builtin SPICE model and compare actual curves/thresholds/currents.
TL072CP assumptions: A0=200k,GBW3MHz,slew13V/µs,Rin1TΩ,Rout50Ω; rail margin1.5V.
Use **legacy TL072**, not modern TL072H20V/µs/5MHz. Official TI SLOS080W July2025
PDF is archived/pinned in provenance. Output resistance/rail macro remain reduced
assumptions; no input common-mode phase reversal, detailed recovery/current limit,
noise/offset/bias/device-charge or component-tolerance calibration claimed.

Original ICL7660S charge pump inverts9V and drives a two-diode doubler. Distinguish
U1's0/+9V domain from U2's negative/boosted domain. Fixed nominal **−8.6/+16.2V**
rails follow the independent annotated analysis, **not** ideal−9/+18 or measured
hardware. Model passive4.5V divider/cap rather than fixing that loaded signal node.
Quantify rail sensitivity/headroom in independent references; pump clock/ripple,
source resistance, sag/startup and supply current coupling are omissions. No
physical measured-equivalent supply claim; domain reduction validity is bounded by
those comparisons and must be reported if insufficient.

Plan one fixed21-node/17-capacitor circuit, four internal amplifier states and
one nonlinear diode voltage. Exact stopped linear reduction, exact zero-R
constraints, bounded5x5 Newton with **24 updates/8 backtracking trials**. First
baseline **4x/193tap FIR**, IEEE exact exponentials; investigate grid refinement
rather than silently choosing16x. State≤12KiB/text≤16KiB standalone ceilings;
initial hard bound4*(1+24*8)=772 candidate evaluations/1544 expm1 per base frame.
Report observed counts and actual full-process host timings. These are not the
future2000cycle/sample hardware admission gate, which remains unmeasured.
No secretly simplified embedded version; optimized embedded approximation is a
separate future decision if this host-fidelity baseline is too costly.

DC≤10µV; device20ppm+10nA; complex AC≤5% through10kHz; ordinary reference
jack RMS≤1mV/peak≤5mV, ordinary stages RMS≤10mV/peak≤50mV. Harsh rail/slew
probes retain all errors; initial hard-stage peak budget200mV/jack10mV with
strictly improving grid refinement, no numerical failure sound. Harmonics,
phase/cancellation, all actual loaded currents/KCL and alias study required.
No allocator/free/lock/IO/JSON/unbounded audio work. Full regressions/sanitizers,
actual pinned M7 audit, original binary/golden preservation. No Engine/patch/pool/
firmware changes or physical CPU/audio/boot/deadline claims.

New permitted DI plan: Guitar-TECHS Player3 **take03**, distinct from take11/take06/
GuitarJam050. Verify archive/original, retain source license/full original and
explicit20s mono16 excerpt (no resampling/EQ/dynamics/gain), add2s tail. Three
low/medium/high Gain conditions, shared healthy/fault source/control/calibration,
raw plus bounded disclosed RMS matching; controlled attacks/releases/long notes/
chord and analysis-only loaded path taps. Founder audition remains a human gate.
