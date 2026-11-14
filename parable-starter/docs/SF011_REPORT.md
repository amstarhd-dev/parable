# SF-011 — original-style Centaur and one clean-gang contact

Experimental standalone Soundforge voice from main
`dbde3438b41ad834ba7801cc111be1c16adfbdad` (independently merged #50/#52).
[Gate 0](SF011_GATE0.md) was committed before native implementation;
[ADR-011](adr/ADR-011-klon-clean-contact.md) records the decision and refinement.
#42/#48 remain open and untouched. No merges, cherry-picks or other PR edits.

## Result and acceptance boundary

One native C++ implementation renders both voices. Only the **P1A clean-side
wiper-to-external-lug contact** changes: ideal0Ω healthy,100kΩ series faulty.
Its track and P1B gain gang still follow the same knob. This conductance is in
the loaded circuit equations, not a clean-buffer gain or random crackle/gate.
Both feedforward paths, nonlinear loading and phase cancellation remain present.
The result is an alternate balance, not complete removal of all clean signal.

Host agreement establishes this **declared reduced circuit model**, not a
calibrated physical Centaur. Founder recognition/preference is pending. The
12x fidelity baseline misses Gate0's initial4x static compute budget by3x;
Seed3 execution/cycles/deadline/stack high-water and hardware admission are
**unmeasured**. No Engine, canonical patch, slots, storage, activation pool,
firmware, living-source or correspondence implementation changes.

The supply work proves the domain distinction and sensitivity of the reduced
macro. It does **not** establish a physically measured-equivalent charge pump
or vintage9V TL072. If the issue requires physical supply equivalence rather
than a bounded, explicit modeling assumption, that criterion remains unfulfilled.
No successful ELF/object compile or simulator exit is used as that proof.

## Pinned source, reuse and physical assumptions

Primary: Martin Chittum's15 April2009 original Centaur **S698** trace,
https://aionfx.com/app/files/schematics/klon-centaur-schematic.png ,138182bytes,
SHA256 `20f736382cd5ccb74d143d519cc9d39135f153ef8422a18d7be175272e83eb39`.
Independent ElectroSmash diagram mirror commit
`ebf6b8604d663c1832d06682e74940830cdc5490`, image SHA256
`69e2ec4527bcb7a7a14d61d4be00b08dc5919e90ea6731b52cd0addafb7ec673`.
Only component facts were transcribed, not artwork/text/layout/unlicensed code.
Original1.8k/4.7k tone orientation, R1=10k/R11=15k/R28=100k and ICL7660S
remain; no later clone/KTR/swap substituted. Source numbering matters:
P1A=clean, P1B=drive, R241=tone100k feedback, R26/R27=output68k bleed.
ElectroSmash calls the gangs RV-GAIN_b/RV-GAIN_a respectively.

The Gate0 table maps all components. In particular R17=27k is **parallel to
C12=27n plus R18=12k in series**, not three parallel parts. The actual passive
Vref27k/27k/47u and all16 signal capacitors are retained. Exact pot endpoints
and wire constraints use prepared rank-one elimination, not arbitrary tiny
resistors. Gain gangs are100k linear, Tone/Output10k linear. External source
is1kΩ and jack load1MΩ, in addition to the original bleed/load network.

Audited ChowCentaur `f3bb633a593b6fbb22a44c1ef9d1dbedbfe92d5b` is credible
prior art. Its separate WDF/feedforward termination assumptions and endpoint
resistor floors do not directly preserve the chosen contact's mutual loading.
Its BSD-3 license is permissive but outside our unchanged MIT/BSD-2 code import
allowlist; **audit-only**, no JUCE/WDF/RNN import. MIT ACME.jl
`3832ccafb952ee2ca0a27286c091d613062c23fd` supplies the diode/finite-pole
foundation, with full notice. Main's owned passive elimination, capacitor
companions, pivot/damping, FIR and host laboratory patterns were adapted.
[Provenance](../third_party/klon/provenance.json) pins research, rights and gaps.

Generic Ge pair: IS600nA,n1.5,T300K,EG.67eV, no RS/Cj/TT. Approximately.35V
at5mA agrees with a published original-unit observation, not our measurement of
a vintage1N34A or an asserted manufacturer model. Compare native exact IEEE
expm1 to independent SPICE builtin diode currents, including negative polarity.

Legacy TL072CP, **not TL072H**: A0=200k,GBW3MHz,slew13V/µs,Rin1TΩ from
TI SLOS080W July2025 typical data. Rout50Ω and1.5V rail margin are explicit
reduced assumptions. No vintage9V calibration, offset/noise/bias, intrinsic
charge, input common-mode phase reversal, output current limit, detailed
saturated recovery or component tolerances. Continuous internal amplifier law:

`du/dt = clamp(wp*(A0*(Vplus−Vminus)−u), ±13e6) − 1e10*(u−clamp(u,lo,hi))`

with wp=2π15 and u relative4.5V. U1 is0/+9V (relative±3V output limit); U2
uses nominal−8.6/+16.2V (relative−11.6/+10.2V output limit). Local self-feedback
is eliminated analytically and exactly; cross-loading and diode coupling remain
in the joint5-unknown Newton solve. External caps use trapezoidal companions,
internal stiff amp states BDF2. No approximate exponential or block linearization.

## Supply-domain investigation

The original ICL7660S switches C21=1u to produce negative C19=1u and drives
C20=1u/D5/D6/C22=1u positive doubler; BOOST ties to9V. C17=47u decouples the
stiff9V source. Pin Renesas **FN3179 Rev7.01,10 February2020**, PDF SHA256
`deab14980df4c62ee9025a0b49e0ea89d33982cd100bdac7cc89ac2ae3567334`.
The offline literal island uses a four-switch equivalent, not a firmware pump.
Its illustrative typical5V values (10k oscillator, BOOST~3.5x,pump=fosc/2,
switch23Ω) are **not calibrated9V values**.1Ω capacitor ESR, diode model and
2.8mA U2-pair load are declared assumptions, not measured signal-dependent load.

Eight independent sweeps vary pump10/17.5/35kHz, switch10/23/50Ω, ESR1/5Ω,
load1/2.8/5mA. Nominal17.5k/23Ω/1Ω gives mean+16.539/−8.712V at1mA,
**+16.066/−8.198V at2.8mA** (~84mVpp rail ripple),+15.553/−7.571V at5mA.
This does not make fixed+16.2/−8.6V universally equivalent. Switching startup
uses zero-capacitor UIC and20ms settling; invalid DC-phase startup runs were
rejected before fixtures. Time-weighted moments avoid adaptive-grid mean bias.

Sixteen audio sensitivity cases compare fixed nominal, ideal+18/−9, sag+15.2/
−7.6 and deliberately wrong single0/9V U2 domain at four signal/control probes.
Nominal/ideal/sag outputs coincide because these probes never hit those U2 rails.
Wrong single9V domain clips the high-Gain/treble-boost probe, changing jack RMS
by30.58mV and peak70.24mV. See [supply plot](figures/sf011/klon-supply-domain.svg)
and `supply-*.csv`/literal netlists. This bounds **rail-only macro sensitivity**;
it cannot establish PSRR, clock noise, actual load/sag, startup or physical
headroom/recovery. These remain explicit physical-fidelity limitations.

## Independent numerical evidence and rejected prototypes

Pinned ngspice44.2 builtin Ge plus independently authored continuous G/C/E
amplifier macro; no native stamps/Newton/output used to generate fixtures.
101 diode points,25 DC contact/Gain cases (nodes/amps/caps),378 complex AC rows,
315 candidate-contact AC rows,20 nonlinear transient cases on four internal
grids,120 coherent harmonic rows and24 supply netlists. All CSVs and literal
netlists have SHA256 pins. Regenerate and replay using the command below.

Initial behavioral SPICE OP/gmin pseudo-transients returned false bias points;
rejected by DC/KCL checks. Accepted DC/AC uses an explicit G/R/C/E linearization,
exactly the same law inside independently verified quiescent slew/rail bounds,
without nodeset. Nonlinear transients start from verified capacitor equilibrium
with UIC, then release all states. Internal reference capacitor/current were
scaled together to1u for conditioning; the continuous law is unchanged.
Accepted transient Gear2,reltol1e-5,abstol1e-10,vntol1e-7,maxstep.5µs checks
finite/full-span results; hardest fault also refines maxstep to.125µs. DC/AC use
1e-9/1e-15/1e-12 tolerances. Reference uncertainty is retained in the manifest (hard-fault jack refinement
0.0004189025V, largest internal-node refinement0.006627288V).
Fresh full regeneration/replay passed with zero numeric difference in all11 CSVs
and identical104 literal netlist hashes.

DC peak deviation3.493e-10V; diode20ppm+10nA gates pass (largest absolute
error1.045µA occurs at large current). Complex AC maximum relative error
**0.6447%**, including real/imag phase and all three loaded summing currents.
At1kHz, low-Gain healthy/fault gain.734896/.215235 V/V; high88.9422/115.352.
The fault changes phase/frequency shape, not a scalar. Gate0 records candidate
0/10k/100k/1M/1G sweeps and why only100k was selected.

Transient errors against continuous SPICE, before /8 software scaling and FIR:

| Internal Hz | All-stage RMS V | Ordinary stage peak V | Harsh stage peak V | All-case jack RMS V | Ordinary jack peak V | Harsh jack peak V |
|---:|---:|---:|---:|---:|---:|---:|
|529200,44.1k×12|.00195401|.00206518|.0852179|.00136801|.000184850|.00811634|
|576000,48k×12|.000880163|.00154481|.065325|.000318781|.000143218|.00459368|
|768000,refinement|.000656649|.00272303|.0515336|.000255814|.0000472318|.00329628|
|1152000,96k×12|.000163931|.00101385|.0233627|.0000586642|.0000517839|.00134424|

Ordinary jack RMS has a separate1mV gate; all-case RMS above includes the harsh
.8V/10kHz slew probe. Other original gates: ordinary stages10mV RMS/50mV peak,
jack5mV peak; harsh stages200mV peak/jack10mV peak. No gate was relaxed.
4x/176.4k and192k failed harsh jack peak at41.39/32.96mV.8x/352.8k still
failed18.02mV (384k7.19mV alone was insufficient). Final12x/577tap symmetric
FIR passes all three sample rates. Failed grids can be regenerated with `--grids`.
Bounded solver failures, invalid inputs and clamps must be zero in ordinary
renders; physical modeled rail/slew encounters are separate counters.

H1/H2/H3/H5/H7 and extrema compare Clip, SumOut, Jack and all three grouped
currents to independent SPICE (5%+100µV voltage/10nA current harmonic bounds).
R16 alone is not a pure dirty contribution: C11 carries clean feedthrough there.
Use drive=C10 current, clean1=R19 current, clean2=−C11current+R17current−C12current,
all signed into the **actual** sum node. Their KCL plus R20/C13 feedback and
finite Rin holds. Three diagnostic transimpedances−392k/(1+s392k820p) reconstruct
**SumOut−SumMinus**, not Tone/Jack. [Phase/current plot](figures/sf011/klon-frequency-phase.svg)
and [loaded waveform plot](figures/sf011/klon-loaded-waveforms.svg) show that distinction.

Coherent alias probe96k/4096frames/bin340=7968.75Hz/.15V,G.7/T1/L.5,2s settle:
healthy1x−80.718 vs12x−155.155dBc; fault−80.260 vs−153.700dBc (74.44/73.44dB
improvement). This is one reproducible probe, not a universal noise/alias floor.
A phase-reset capture kink was caught and corrected in the probe; DSP was not
changed to improve that metric. `science-accuracy`, `stage-sweeps`,
`harmonic-comparison` and `alias` CSVs retain actual measurements.

## Downloadable listening package

CI publishes **parable-klon-auditions**,30day retention. PR links identify the
actual checked-out source, branch head, job and ZIP digest. Start
`14_medium_healthy_matched.wav` / `15_medium_contact100k_matched.wav`, then raw
`11/12`, low`04/05`, high`24/25`.18 musical files are three Gain groups ×
dry/healthy/fault ×raw/matched, each20s plus2s tail/48kHz. Gain0/.5/.95,
Treble.5,Output.85/.5/.5; pair controls/calibration identical. Across-group
Output differences are disclosed, not evidence of Gain-only loudness tracking.

New **Guitar-TECHS Player3 directinput_03**, not050/take11/take06. Same player/
dataset, different performance. Zenodo record14963133 revision12 exposes no
semantic version; do not invent one. Archive129505089bytes SHA256
`033489e22600751fb5a1633e7d856b901c6782e0486fa02135e830780d9dbfe2`;
member `P3_music/audio/directinput/directinput_03.wav`, stereoPCM24/48k,
1406000frames SHA256
`84980b5468f3315171e4e546a1d38d54e4c368ccb5fee72c759e0915f346fad0`.
Both channels sample-identical. First20s, channel0, nearest/ties-away24→16,
no resampling/EQ/dynamics/gain; prepared SHA256
`75ae5e73c72e6774fb75e6318dc91ba66347fb65290949e947144033eb648f52`.
Full original and conversion receipt retained. CC-BY4.0 attribution names all
five authors in [AUDIO_NOTICE](../third_party/klon/AUDIO_NOTICE.md).

Main input is calibrated to.2modelV peak; both dry and processor use output
Volts/8, raw offline gain1. RMS matching requests−23dBFS with.89peak/16x gain
cap; common target is lowered when needed. Whole-file RMS, not LUFS. No hidden
limiter or gain normalization in raw files. Measured final12x raw results:

| Gain | Healthy/fault RMS | Healthy/fault peak | Healthy/fault crest dB |
|---:|---:|---:|---:|
|0|.00437313 / .00431336|.0266697 / .0221109|15.705 /14.196|
|.5|.0248333 / .0254411|.0995409 / .102540|12.059 /12.107|
|.95|.0611787 / .0657946|.150438 / .153440|7.815 /7.355|

RMS changes−1.37%,+2.45%,+7.54% do not establish an audible improvement. Balanced
sine/SPICE phase and path-current data establish electrical change; founder
listening establishes whether it is useful. No forced gating/grit/pleasantness.

Four additional raw music files probe.04/.4modelV at medium controls.20 controlled
files cover attacks, long note/release, impulse/release and polyphonic chord,
input/raw/common-gain pairs.6`analysis/` WAVs are loaded pre-Tone feedback-voltage
contributions with one common safety gain, **not isolated clean/dirty outputs**.
They omit final Tone/FIR; last internal phase11 differs from decimated phase0
output (nominal48frame combined FIR delay). Total48 native WAVs; with exact
original and prepared input, **50 WAVs**. CSVs retain actual nodes/17caps/4amp
states/diode currents/KCL/path residuals,64-bin histograms, spectra, envelopes,
physical rail/slew counts and numerical containment. All20 render conditions
passed zero numerical failures/clamps and bounded KCL/path reconstruction.

## Safety, compute and ARM evidence

Stopped prepare validates all controls/rates and preserves old state on failure.
No concurrent prepare/reset/audio ownership is offered. Invalid input returns0
without advancing history; finite input clamps±2modelV; output containment±1.
Counters saturate. Exact reset, Level0 wire, silence/note decay, rate/control
endpoints, positive/negative/transient/chord/extreme probes and callback guards
are tested.24 Newton updates,8backtracks,5×5 bounded elimination. No callback
allocator/free/locks/IO/JSON; guard self-test verifies interception.

Measured full native processor pass, GNU14.2 Release/AMD EPYC9V74 Linuxx86_64,
shared host,22s per guitar setting, separate exact reset/repeat excluding prepare/
traceIO/analysis but including diagnostics and FIR:
low healthy/fault **9.65/9.38µs/frame**, medium11.45/11.33, high12.15/12.33.
Mean candidate evaluations/internal sample1.949/1.950,2.652/2.652,2.893/2.894;
observed maxima2/2,4/4,4/4. Host timings are measured wall time, not M7 estimates
or a comparison against an equivalent optimized reference. CI records its own.

Actual pinned xPack GNU Arm Embedded14.2.1-1.1 Cortex-M7 hardfloat/O2 object:
**9808B text/0data/0BSS,11808B complete processor state**. Original16KiB text/
12KiB state ceilings pass. Engine/bundle/Runtime/RAT11040/11288/45224/10752B
unchanged. Undefined symbolscos/sin/expm1/memcpy/memset; cos/sin are prepare-only.
Audio disassembly calls only bounded native functions/expm1/memcpy/memset,
including resetCircuit on visible numerical containment; no prepare/matrix
construction/full reset/allocator/IO/locks. No pow/approximate exp.

Static audio frames process48+sample1104+solve728+evaluate320+reconstruct48 =
**2248B application chain**, excluding libm/memcpy/ISR/high-water. Stopped prepare
12184+matrices7528+constraint376=20088B chain, before helpers. Large stopped stack
needs explicit future integration budgeting. Per base frame hard ceilings:
2316 candidate evaluations,4632expm1,288 solves of5×5 systems,1154 FIR coefficient
multiply-adds.12 internal steps,17 cap histories,21 nodes. Initial4x compute bound
was3× smaller. These costs make this a **host-fidelity research baseline**;
future2000cycle/sample target admission is not established. No Klon firmware
link, physical boot/audio, target cycles/deadlines/latency/power/noise or SRAM/
SDRAM/stack high-water measured. Existing firmware links contain no Klon.

## Reproduce from repository root

C++17,GNU14.2,CMake3.31.6,Python3.12.14. Existing CMake downloads pinned JSON;
no new production dependency. Optional figures use Matplotlib only offline
(version recorded in figures/PLOT_ENVIRONMENT.txt), not CI/build/runtime.

```sh
cmake -S parable-starter -B build/sf011-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/sf011-debug -j2
ctest --test-dir build/sf011-debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B build/sf011-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/sf011-release -j2
ctest --test-dir build/sf011-release --output-on-failure --no-tests=error -j2
python3 parable-starter/tools/soundforge/validate_registry.py
cmake -S parable-starter -B build/sf011-asan -DCMAKE_BUILD_TYPE=Debug \
  '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -O1' \
  '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined -no-pie'
cmake --build build/sf011-asan --target parable_klon_test parable_klon_lab -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build/sf011-asan --output-on-failure --no-tests=error -R '^klon_'
python3 parable-starter/tools/klon_spice_reference.py --ngspice /path/to/ngspice44.2 \
  --work build/sf011-spice --output build/sf011-reference \
  --check-existing parable-starter/tests/fixtures/klon
curl --fail --show-error --location \
  https://zenodo.org/api/records/14963133/files/P3_music.zip/content \
  --output build/P3_music.zip
python3 parable-starter/tools/klon_input.py --archive build/P3_music.zip --output build/klon-input
./build/sf011-release/parable_klon_lab \
  --input build/klon-input/take03-20s-mono16.wav --output build/klon-auditions
python3 parable-starter/tools/check_klon_arm.py \
  --compiler /path/to/pinned/arm-none-eabi-g++ --output build/klon-arm
python3 parable-starter/firmware/build.py
```

ngspice44.2 Debian package:
https://deb.debian.org/debian/pool/main/n/ngspice/ngspice_44.2+ds-1_amd64.deb ,
SHA256 `4d76721dd91416055b1088cfc58626d4db47b91afda7c73258c0968ba6e4d24e`.
Only reference regeneration needs it. Existing workflow retains full host suites,
23 prior ASan/UBSan tests and four prior TSAN tests; adds two focused Klon
sanitizer tests, pinned ARM audit and repeatable real-DI artifact publication.
Host/integration timeout60min accommodates the existing slow Debug suite plus
new scientific tests. No old test filter was removed. Full JSON/internal-flash
size overflow remains the existing deliberate non-gating diagnostic, not a new
Klon link failure. Local complete suites:36/36 Debug1984.66s and36/36 Release360.79s;
focused ASan/UBSan2/2,1631.08s. Debug and sanitizer runs preceded the
additional explicit ordinary-jack RMS assertion; the complete Release suite
includes it. Final focused reruns and final-head CI outcomes are recorded in
the PR, separately from these local full-suite timings. Registry11 assets passes.
All270 protected old source/header/firmware/fixture hashes and27 fresh canonical/
living outputs remain exact. Original16 RAT golden outputs pass. Actual CI
outcomes and downloadable artifacts are in the PR.

## Changed files, measured reuse and next gate

- New native header/source/device helper: this one fixed circuit/contact.
- New independent reference/supply scripts, pinned CSV/netlists: external numerical
  truth, domain sensitivity and failed-candidate audit; no old goldens refreshed.
- New runner/stimulus/input/test/ARM/optional-plot tools: same-C++ listening,
  physical measurements, rights/transformations, safety and resource evidence.
- CMake, two workflows, registry/test: standalone targets and evidence automation.
- Gate0/ADR/report/README/status/notices/static figures: reviewable decision,
  rights and specified/implemented/host-tested/hardware/listening distinctions.

Unchanged reuse measured as physical source lines: PCM83,analysis58,excitation31,
callback guard87,SPICE process wrapper20 (279 total, no shared refactor). Adapted
native/header/device319lines; lab97; independent reference/supply281; input39;
ARM39; plot45; stimulus25; science test136; CLI test49. Dense
physical lines are not an engineering-effort or semantic-reuse percentage.
No manufactured hours-saved/96-circuit throughput claim. First8x prototype WAVs
were generated before the final44.1k grid exposed failure; final12x WAVs replaced
that candidate after scientific acceptance. Rework comprised rejected4x/8x grids,
SPICE conditioning/DC method, supply startup and the phase-reset probe; all are
reported rather than counted as reuse success. Future models reuse laboratory
facilities, source discipline and these reductions where scientifically applicable.

Next smallest ticket: **founder blind/raw+matched listening review of three Gain
pairs**, retaining recognition/usefulness verdict separately from loudness.
If approved, commission a separate measured target-CPU/admission investigation
and physical supply/legacy-TL072 calibration before proposing embedded
approximation or Engine integration. Do not silently simplify this reference,
expand architecture, invent correspondence semantics or treat review as approval.
