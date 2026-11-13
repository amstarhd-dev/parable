# SF-010 Gate 0 — TS808 / one open feedback diode

2026-11-13 UTC. Started from current main
`a17f64fe222dc5c9867c693fb3b0e4e69c71f465`, before native edits. Read project
AGENTS/Constitution/README/status, Soundforge recipe/baseline, RAT/CMOS/Ge models,
diodes/FIR/PCM/reference/guards, ADR001/PAR008/PAR011 and SF009 report/ADR/source
as read-only prior art. Root AGENTS and frozen Lexicon absent. #42/#48/#50 remain
open, heads057312b/9f8548c/bdecc9d; no import, cherry-pick, merge or edits.
Muff is **not on main**, so its code/tests are not this baseline's dependencies.

Clean GNU14.2/CMake3.31.6: **30/30 Debug123.62s / Release72.04s**. Commands:

```sh
cmake -S parable-starter -B /workspace/work/sf010-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/sf010-Debug -j2
ctest --test-dir /workspace/work/sf010-Debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /workspace/work/sf010-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/sf010-Release -j2
ctest --test-dir /workspace/work/sf010-Release --output-on-failure --no-tests=error -j2
```

Captured131 include/src/firmware/fixture hashes and27 canonical/living output
hashes before edits. Old protected goldens will not be refreshed.

## One schematic, permitted reuse and exact orientation

Select ElectroSmash's **TS808 true-bypass/effect-on signal path**, not a mixed
TS9/clone. Mirror `mstratman/electrosmash-pdf-backups` commit
`ebf6b8604d663c1832d06682e74940830cdc5490`, file
`images/tube-screamer-analysis/tube-screamer-true-bypass-schematic.png`, Git blob
`6aaba1337b9a0ca4e8c717784e77913a36040a9d`, SHA256
`87399df12eb0c2317238d363893da1eab9db44f16df353f31ddcc34b866c42a9`.
Drawings inspected, numerical facts transcribed only; no artwork/text/layout/code
import or diagram-license inference. Archive analysis clarifies RA=0Ω, RB=100Ω,
RC=10kΩ. C1 is20nF in this drawing, not silently substituted22nF. JFET/toggle
bypass omitted as the selected drawing does; no bypass implementation claimed.
R.G.Keen's original Geofex analysis independently corroborates feedback clipping
and selective gain/tone principles; not a second BOM.

Drawing **D2 anode=clip op-amp output, cathode=inverting input**; D1 opposite.
Open **D2** completely. This orientation, not the article's conflicting positive/
negative prose label, determines the test. R6/P1 and C4 remain, so open D2 never
means infinite gain. No other fault, noise, gate, mix, gain or severity parameter.

MIT ACME.jl `3832ccafb952ee2ca0a27286c091d613062c23fd` diode/bjt and finite-gain/
GBW opamp equations are reusable foundations; retain its full notice already on
main. Specialize nominal silicon and one coupled circuit, not Julia/framework.
Audited BYOD `1cf22b6ac802b9dc33cfc9f8dd6af5b3c3e40bc9` TubeScreamerWDF and
TS-808-Ultra `1a9dfe314f7f85083a2f51ec0b496226afdfaccf`: GPL3; no import.
BYOD's inspected stage assumes fixed op-amp parameters/symmetric diode adapter;
adapting it would not settle the requested one-open/full-path experiment.
Reuse main's owned pivot/damping/companion/FIR, existing PCM/stats/excitation,
callback guard and unchanged `run_spice` wrapper. No new generic netlist framework.

## Physical model and pre-code independent probe

Retain both emitter followers, full loading and nine signal capacitors. Nominal
NPN transportIS1e-14A/BF350/BR4, 300K; diodeIS2.52nA/n1.752. Finite op-amp
A0=100k, GBW3MHz, slew1V/µs, differential input5MΩ, output75Ω. Gain/GBW/slew/
input resistance derive from legacy NJM4558 typical data;75Ω and±3V relative
output swing at9V are disclosed reduced-model assumptions, not a measured vintage
JRC4558 or9V manufacturer guarantee. Rail restoring rate1e9/s permits≤1mV static
overshoot at max slew. Internal dominant-pole state, slew and rails stay **inside
feedback**, not a clipper after the amplifier. Supply9V/virtual-ground4.5V ideal;
divider sag, intrinsic BJT/diode capacitances, input bias/offset/noise/current limit
and detailed saturated recovery omitted. No commercial-unit fidelity claim.

Independent literal ngspice44.2 netlist uses builtin NPN/diodes and a separate
behavioral current+capacitor op-amp macro, not native stamps. Pre-code DC:
Q1/Q3 base4.00860989V/emitter3.38192036V; both op-amp signal biases4.5V; opening
D2 leaves DC unchanged. At.2V/997Hz, Drive/Tone/Level.5, healthy clip output
3.886..5.114V/jack±.193V. Open D2: clip3.890..7.445V, jack−.552..+.777V in the
40ms probe. Finite positive rail and asymmetric feedback follow actual KCL.
Strict transient reltol1e-6 aborted near26ns for the fault; rejected. Gear2,
reltol1e-5/abstol1e-10/vntol1e-7 completed40ms; full-span and step-refinement
checks are required in final references. A truncated reference is never sound.

## Admission, bounds and new guitar

Standalone only: expected12 nodes/7 nonlinear unknowns,9 external cap histories,
2 op-amp states; exact stopped linear elimination, bounded24 Newton updates+
final evaluation/internal sample. Fixed4x/193tap FIR reused from main. Hard8KiB
state/16KiB ARM-object text ceilings; ≤100 device evaluations/base frame and
≤600 exponential calls if the fixed solve is retained. These are budgets/bounds,
not target cycles or proof of future2000cycle admission. Measure full host process,
actual pinned M7 object/layout/calls/static frames; all seven existing firmware
links remain independent and unchanged. Engine/bundle/Runtime11040/11288/45224B
must not grow. Preparation and resets belong outside concurrent audio processing.

Accuracy gates: DC≤10µV, device≤20ppm+small absolute floor, all-stage192k transient
RMS≤10mV/peak≤100mV with refinement improvement; complex AC≤5% up to10kHz;
record actual tighter errors, harmonic/asymmetry/alias/rail/slew behavior, zero
fallback/containment in ordinary references/listening. Run44.1/48/96k, endpoints,
reset/rejected prepare, finite extremes, long notes/silence/guards, full host
regression and focused sanitizers. No shared-state edits; old TSAN coverage retained.

New permitted DI: Guitar-TECHS v1 **Player3 performance06**, distinct full32s
performance from Muff's take11 and rejected GuitarJam050. The pinned record only
offers P3 music; this is a new take, not a different player or falsely new dataset.
CC-BY4.0, attribution/source/license retained. Same archive SHA
`033489e22600751fb5a1633e7d856b901c6782e0486fa02135e830780d9dbfe2`;
member `P3_music/audio/directinput/directinput_06.wav`, original stereo24/48k,
1536000frames, SHA `2fd88dc725f475e13686f50ce6c88e7fed3d6b635b62147a69d42e29a5dd6254`.
Identical channel0, nearest/ties-away quantize to mono16 without trim/resampling/
EQ/dynamics/gain: SHA `58c0359535334e5ac8734d6bb9ece119133278f869f4897729418996d2e75927`.
Founder judges pleasantness; no agent subjective approval. Same calibration/raw
gain for both, bounded disclosed offline RMS match; keep exact original and receipt.
