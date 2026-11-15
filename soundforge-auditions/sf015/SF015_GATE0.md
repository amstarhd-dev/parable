# SF-015 Gate 0 — original-FZ-1-style, before native implementation

2026-11-15. Experimental standalone Soundforge specimen; founder listening and
independent review required. No Engine, patch, firmware, correspondence or hardware
change. The complete issue is #60. Constitution, AGENTS, README/status, Soundforge
rules and SF003/009/010/011/014 records were read. Project AGENTS exists; root
AGENTS and the complete frozen correspondence Lexicon are absent, not recreated.

## Baseline and branch boundary

Base `af159d833bdd0b0e17ccc59fcf7edfa506ed98f5`, current main. GitHub reports PR59
already merged independently before this work; this job did not merge, cherry-pick
or modify it. Its now-main source is preserved. Open SF012 PR56 and parked issue57
are excluded. Before any edits: clean Debug **38/38, 2439.85s** and Release
**38/38, 258.38s**, GNU14.2.0, CMake/CTest3.31.6, C++17. Commands:

```
cmake -S parable-starter -B BUILD -DCMAKE_BUILD_TYPE=Debug
cmake --build BUILD -j2
ctest --test-dir BUILD --output-on-failure --no-tests=error
# Repeat with Release and a separate BUILD directory.
```

This environment used `-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src`
for the unchanged pinned3.11.3 dependency. 427 pre-existing implementation/header/
firmware/fixture files were hashed before edits; preservation will be checked.
Current-main Actions jobs failed at startup before checkout/build/test; local
passing tests do not substitute for independently running CI. No CI repair or
account configuration is part of this circuit ticket.

## Borrow first and inspect actual sources

* ACME.jl `3832ccafb952ee2ca0a27286c091d613062c23fd`, actual `src/elements.jl:bjt`
  and MIT license inspected. Reuse main's existing `germanium_device.h` GP evaluator
  unchanged; adapt its two-junction-port resistance representation narrowly. No
  Julia, generic network framework, open-PR optimization or universal BJT library.
* Greg Whitehead `germaniumbjts` `7838a40f939eaee39b37afef1dac031ef45c276a`, actual
  parameter library and MIT license inspected; published measured AC128 parameters
  from Holmes/Holters/van Walstijn DAFx2017 Table1 and equations1,4–7.
* Exact original-topology prior art `grwhitehead/us3213181`, latest
  `661e93501aaefea5d9389ea68966b47cc496fd37`: actual netlist, web implementation and
  MIT root license inspected. Newer Gert Willmann2021 2N270 card has separate
  attribution without independently established import rights: **audit-only**.
  Earlier `4c97863186a248d4e0d5b1d2933e273915c03748` uses AC128, but deliberately
  retunes 10k/2.2k/10k resistors to7k/1.4k/3k. Those tuned values are **not** borrowed.
* Swell's simulator explicitly restricts reuse: audit-only, no code, presets,
  model parameters, audio, text or drawings imported. RT-WDF license/framework
  conflict: rejected. Existing owned Muff passive elimination, fixed solve idiom,
  FIR, Soundforge WAV/statistics/excitation and callback guard are reusable.

Full URLs, hashes, license boundaries and notices: `third_party/fz1/`.

## Pin one falsifiable circuit

Primary Snoddy/Hobbs patent US3213181, filed1962-05-03, issued1965-10-19.
Scan https://patentimages.storage.googleapis.com/54/2d/4f/efb3c42526c46c/US3213181.pdf
SHA256 `c910500639caac4c784ee811bf4d337a6578785128fd8612f58558094f6f31bf`.
Actual scan and Amplified Parts original3V Figure2 viewed; only component facts
transcribed, no artwork copied. Literal executable netlist: fixtures/fz1/nominal.cir.

Positive ground0, battery negative **−3V**, allowed stopped diagnostic magnitude
2.7–3.3V. Physical PNP Q1 collector→−3V, emitter→E1, base→B1; Q2 E→0,B→B2,C→C2;
Q3 E→0,B→B3,C→C3. Native equivalent coordinates may negate every physical voltage.

| Connection | Part |
|---|---|
| source→input capacitor | declared1k source +100k R1 |
| capacitor→B1 |10nF C1 |
| B1→0; E1→0 |1M R2;10k R3 |
| E1→B2 |20µF C2, positive sideB2 |
| −3V→B2 |470k R9, the **only** fault candidate |
| B2→0 |2.2k + (22k || Attack rheostat0…50k) |
| C2→−3V; C2→B3 |1.5k R6;20µF C3 positive sideB3 |
| B3→0; C3→−3V |10k R7;10k R8 |
| C3→Volume top |3.3nF C4 |
| Volume top→0 |**56k shunt R10** |
| Volume top→wiper→0 |500k total, loaded by1M amp at wiper |

Attack is electrical resistance fraction, not an asserted mechanical audio-taper
calibration. Volume is electrical divider fraction, including exact endpoints.
Stopped diagnostics source100…10000Ω, amp100k…2M. No pickup inductance/cable model:
this is a finite Thevenin-source loading test, not calibrated pickup interaction.

**Discrepancies resolved before coding:** patent text470Ω versus independently
corroborated470k:470Ω saturates Q2 (collector≈−.01V), whereas470k permits signal
headroom. Amplified Parts BOM erroneously lists two22k and omits100k; drawing and
patent support input100k and one22k. Issue prose calls output56k “series”; both
source drawings show **shunt**. Use actual drawing connections. Patent omits Volume
value; original-unit accounts/Amplified Parts support500k. FZ1A1.5V,1µF coupling,
10k Q2 collector and50k Volume is a later circuit, explicitly excluded, as are
silicon FZ1B and reissues. Patent illustrative3.1V/DC points are not measurements
of a completely specified transistor sample. No claim every vintage unit agrees.

## Germanium model choice and rejected calibration

Use unchanged, **measured AC128 template**, all three devices, at Vt25.5mV
(**22.765212°C**, computed exactly in generator). Not an accurately measured2N270.
IS20.66µA, BF229.6, BR14.66, NF1.133, NR1.140, VAF19.68V, VAR88.28V,
IKF.463A, IKR.2415A, ISE2.190µA/NE1.796, ISC7.546µA/NC1.364;
RB1.885Ω, RE.3064Ω, RC1.727µΩ; constant CBC100pF, CBE0.
Complete finite-beta reciprocal GP, Early/high-current/leakage terms retained.
Built-in SPICE PNP is independent implementation; EG.67eV, XTI3 offline thermal
sweep, nominal temperature explicitly matched to Vt (not silently26.85°C).
Omit nonlinear charge/storage, shot/thermal noise, device heating and calibrated
unit spread. Constant CBC is a declared approximation, not a measured lifetime.

Unmodified OC44 and AC128, and a patent-DC-informed three-leakage fit were probed.
OC44 gives very different bias; fitting three ISC values reproduces patent's
three illustrative DC numbers but is underidentified and supplies no independent
proof of a real2N270. Reject that fit as nominal rather than tune towards a desired
sound. Keep original circuit parts and publish the AC128 bias discrepancy:
nominal Attack.5 Q1 B≈−1.9870V/E≈−1.9250V, Q2 B≈−.07394V/C≈−2.53784V,
Q3 B≈−.04688V/C≈−1.77364V. Thus **documented original-FZ-1-style** only.

## Independent fault and decay evidence before native code

ngspice44.2, exact Debian executable/package pin already used by the laboratory;
package SHA256 `4d76721dd91416055b1088cfc58626d4db47b91afda7c73258c0968ba6e4d24e`.
Built-in PNP, matched TNOM/TEMP, Gear2 transient, finite terminal resistances and
constant CBC. DC54 cases: both feed states, Attack0/.5/1, supply2.7/3/3.3V,
ISC×.5/1/1.5. Separate5/nominal/40°C offline sensitivity, plus470Ω/470k/1M/2.2M/open
candidate sweeps. Generator and fixtures are committed before final native DSP.

Independent full transient110Hz plucks (.2V,10ms onset,120ms exponential decay),
0.85s duration, Attack.1/.5/1:1M and2.2M changes modest, **open470k feed selected**
as the clearest one-component endpoint, not multiple damage. At Attack.5, B2 moves
−.07394→−.05258V and C2−2.53784→−2.76932V. Feed current magnitude6.2257µA→0;
Q2 collector current308.1→153.8µA approximately. At Attack1, independent full-pluck
RMS drops .05975→.05206V; peak .8122→.6478V. Q3 itself is unchanged but its transient
operating point moves through C3. These are nonlinear waveform/bias differences,
not merely output attenuation. Final reference regeneration uses tighter4µs steps.

**No abrupt hard gate is established by these probes.** Both healthy and faulty
have a finite linear tail; do not call low-level decay a zero-crossing gate, add
an envelope, or claim the founder's desired musical character has already passed.
Compare actual take03 and let listening decide. If it lacks the requested short
trumpet character, explicitly report that musical objective unproven/failed.

## Numerical/cost alternatives and predeclared gates

Select a fixed six-junction bounded Newton solve after exact passive elimination,
with ACME terminal-resistance matrix and four external + three CBC histories.
No general graph/network framework. Borrow the owned Muff pivot/precharge idiom.
A memoryless whole-circuit lookup loses interstage DC blocking/20µF memory and
cannot represent pluck bias recovery; reject. WDF still needs a coupled nonlinear
junction solve; no existing licensed drop-in exact3PNP FZ1 implementation was
found. A two-voltage device table257²×six double fields×three exceeds9MB versus
12KiB state goal. Reduced leakage-free Ebers-Moll discards the defining bias and
is not accepted merely for speed. Full GP reference is retained unchanged.

Compare1x/2x/4x complete processors before choosing normal audition sampling;
no fixed factor is a prerequisite for target admission. All costs measured on host,
ARM object/calls/frames measured separately, target cycles explicitly unmeasured.
Predeclare, **do not weaken after seeing results**:

* Loaded DC node error≤2µV; terminal current error≤2nA; DC KCL≤2nA across declared
  fixtures. Derivative finite differences relative≤1e-4 +1e-9A/V.
* Independent192k transient: jack RMS error≤2% +20µV absolute RMS; each node
  RMS error≤2% of its AC RMS +.5mV. Entire pluck/capacitor recovery, not last cycle.
* At44.1/48/96k, both states, coherent≈1.76/3.52/8k and guitar-like chord against
  high-rate exact native reference: relative RMS≤5% (8k≤10%); strongest false
  nonharmonic spur below−45dB relative reference RMS. Report misses, do not hide
  them in broadband aggregate or change the reference to the approximation.
* Native reference domain input±.5V; extra input containment is counted.
  Invalid/NaN/Inf becomeszero, denormals finite; controls reject unchanged.
  Silence output≤1e-8 normalized; deterministic reset/precharge. No failed solve,
  input/output containment in ordinary guitar/electrical tests. All rates/extrema.
* Bounded≤24 Newton updates/internal sample and deterministic bounded damping;
  DC prepare≤120 updates. Residual≤1e-9V at runtime,≤1e-12V stopped.
  Complete instance≤12KiB, ARM text≤16KiB, data/BSS0. Callback no heap/IO/locks/
  exceptions; evaluate with existing interposition guard and ARM calls/frames.
* **Future** target ceiling2000cycles/base sample at48k is an admission question,
  not a claim exact exponentials meet it. Do not trade away physics or report
  host speed as Daisy runtime. Cost may make this a research-only specimen.

Next smallest change: implement this fixed stopped-prepared circuit, compare its
DC and full transient against independent built-in PNP, then render actual guitar
immediately. No unrelated processor or architecture work.

## Post-Gate0 findings (thresholds unchanged)

The independent built-in SPICE reverse continuation differs from full exponential
GP; its2µV DC gate misses. Additional explicit-equation SPICE MNA, preserving the
published law, agrees within7nV. Full-history fixtures were strengthened with a
2049-sample192k cycle window and validated span before interpolation. Tiny reference
RC shorts and bounded300s process deadline are reported in SF015_REPORT.

4x misses the false-spur limit; selected8x385-tap playback clears it against a16x
reference. Scientific1x rate ceiling1.536MHz supports that offline comparison;
normal audio rates remain44.1–96k. Complete state5136B/object7720B fit initial
ceilings. Original full-band RMS gate still misses12/24 cases; strict command exits1.
No threshold was changed to certify the result. Abrupt healthy gating remains
undemonstrated. Founder listening and bandwidth acceptance are unresolved.
