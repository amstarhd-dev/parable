# SF-012 Gate 0 — moving ferric tape / one weak-bias driver

2026-11-14 UTC, before native edits. Main
`c3aaca164e7a3f7d5b0106b3dd8dc89b7e1ef940` is the founder's merge of #54;
its tree `40b57915eb1bd001d8dd1f0d4a40187bcf5a034d` equals the audited SF011
head. #42/#48 remain open/unmerged at057312b/9f8548c, untouched. No merge,
cherry-pick or other PR update. Branch sf-012 starts from that main.

Read project AGENTS/Constitution/README/status/kickoff, Soundforge recipe/baseline,
SF006 medium/ADR/code, SF009/Muff, SF010/TS808 and SF011/Klon decisions/reports.
Root AGENTS and frozen Lexicon are absent, not fabricated. Issue55 authorizes
this standalone experiment, not Engine/patch/firmware/correspondence integration.
The founder addendum **overrides the new-DI request**: retain identical take03.

## Clean baseline and preservation

GNU14.2/CMake3.31.6/Python3.12.14, fresh build directories, no repository edits:

```sh
cmake -S parable-starter -B /workspace/work/sf012-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/sf012-Debug -j2
ctest --test-dir /workspace/work/sf012-Debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /workspace/work/sf012-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/sf012-Release -j2
ctest --test-dir /workspace/work/sf012-Release --output-on-failure --no-tests=error -j2
```

**36/36 Debug1433.97s and36/36 Release250.49s.** Snapshot389 old source/header/
firmware/fixture hashes plus27 fresh canonical/living outputs before edits.
All original goldens stay fixed. Retain downloaded/verified SF011/SF010/SF009
audio and ARM archives for byte comparisons. Generated duplicate build/compiler
caches were removed to recover workspace space; source, research and verified
archives/proofs remain. The identical pinned ARM compiler remains in par005-fresh-cache.

Retained clean-tree actual M7 resources: Engine11040B/bundle11288B/Runtime45224B,
RAT10752B; Klon9808B text/11808B state, TS8089096B/8104B, Muff7008B/8832B.
Seven production firmware links remain95220/219720/219720/95356/51592/109600/
109784B load spans. Known full-JSON/internal-flash probe overflows88800B,
explicitly non-gating. These links contain no new tape-saturation processor.
No target timing, available CPU/RAM, boot or audio measurement is inferred.

## Borrow-first selection

- **Isaac Kramer, python-preisach**
  `0b27d738319e78160236068192aa7be3058bbbc9`: read LICENSE.txt(MIT2023),
  README, preisach/preisach_python.py and weights.py. Adapt the actual directional
  relay threshold/hold and positive-weight normalization foundation. Its original
  application is soil hysteresis, not a calibrated tape engine. Remove NumPy,
  allocation/concatenation/NaN grids from native code. Full MIT notice retained.
- **Roman Szewczyk JAmodel** `4bbe905d420ce27c66ee93d8b25ef44b553f9175`:
  MIT LICENSE plus isotropic dMdH/Mah_iso/dMah_iso/JAsolver read. Credible alternative;
  branch-wise ODE solvers/numerical Langevin derivative and unknown tape fitting
  add numerical/calibration work. Select a positive Preisach population with an
  explicit passivity proof, not an unsupported claim that all JA models fail.
- **kiluam/Preisach-model-of-hysteresis**
  `fcbd61293300b7c949a300c7fabd97750fb6c9d7`: MIT, actual source inspected.
  It is interactive reversal-stack bookkeeping with an unbounded input loop,
  not a ready weighted magnetic tape model. Audit-only; no such loop imported.
- **CHOWTape/AnalogTapeModel** `604372e4ffd9690c3e283362e4598cb43edbb475`:
  GPL3 code/notes are audit-only. The separate2019 DAFx paper is CC-BY3 research;
  its physical head/bias/playback equations can be cited without importing GPL DSP.
- **Faust hysteresis.lib** `452a85cc98068a05c30803a4485d94c2ab3f9636`:
  actual LGPL2.1 source with generated-Faust-code exception, not MIT. Audit-only
  under unchanged MIT/BSD2 import policy; acknowledge the exception accurately.
- Infinite Digits' public Tape explanation describes a related moving-Preisach
  implementation, but supplies no imported source license. Reference-only,
  no commercial code, curve fitting, artwork or product calibration borrowed.

Reuse unchanged PCM83/analysis58/excitation31/guard87 lines; adapt existing owned
FIR/FFT/CLI/input/ARM/evidence conventions. Reuse SF011's exact source-preparation
function and attribution. Do not modify the SF006 echo/erase model or use its
SoftClip/feedback/retained-loop mechanism as magnetic saturation.

## Sources and one declared reduced recording path

McKnight, *Biasing in Magnetic Tape Recording*, Electronics World, August1967,
PDF395546B SHA256 `94ea85389578bc48136aee42dbe45d38a0c89d4d26d6aa855df0611f8d8a569a`:
https://cdn.shopify.com/s/files/1/0720/0627/1232/files/mcknight_biasing.pdf?v=1763068428 .
Its Fig1C distinguishes moving tape where audio AND bias fields decay. Lower
bias can increase sensitivity while reducing distortion headroom; it need not
simply mute the output. Its ideal head plus integrating-amplifier discussion
supports the explicitly reduced playback below. No dropout/noise decoration.

RTM2022 SM911 published scales, PDF545700B SHA256
`7ed8d9562a278184637a1519702d0fc89da1d554917f561ac51e76448fb9d5d4`:
https://www.duplication.com/resources/rtm/Data%20Sheet%20Info/fiche_sm911_rtm_2022.pdf .
Use25.5kA/m coercive scale,145mT saturation remanence,16um coating,15ips,
7um record/3um playback gap and320nWb/m flux reference as documented scales.
This is **not a fitted SM911/Studer model**: switching distribution, head efficiency,
turns/spacing, current calibration and electronics below are declared assumptions.
Do not substitute the16um value with an earlier guessed coating thickness.

Chowdhury2019 DAFx paper SHA256
`a915d63daee36556a4f267d9a52f07b1000988ecb48013589d926951a10d613c` and
Tannous arXiv1504.07625 retrieved PDF SHA256
`6026cdfeac13ea37e29d7383b4fecedb6d7b376e53635532a9f4e782090c7f56`
support Karlqvist field/reciprocity and their limitations. No diagram/text copied.

Select a **fresh, ideally erased moving parcel for each output position**.
For a parcel finishing at time t, traverse x=-40..+40um over209.974us at
v=.381m/s. Its field is

`H(tau)=K(x,8.3um)*(N*eta/g)*(0.010*recordV(t+tau)+Ib*sin(2*pi*100k*(t+tau)+phi))`

where `K=[atan((x+g/2)/y)-atan((x-g/2)/y)]/pi`, g=7um,N=300,eta=.6,
tau=-209.974..0us. y=.3um spacing+8um mean coating depth is an **effective-depth
assumption**, not a depth-resolved or vector/interacting-particle solution.
Both audio and bias shrink in the trailing field. Parcels never recur or feed
back. Audio history only covers this short traversal and causal linear filters.

Material: eight positive coercivity populations r=15/18/21/24/27/30/33/36kA/m,
weights proportional to Gaussian(mean25.5k,sigma6k); Gaussian switching centers
sigma12kA/m. Relays switch up at center+r, down at center-r and otherwise hold.
Their ordered boundary p updates as `p=clamp(p,H-r,H+r)`; integrate the continuous
center distribution as `M/Ms=sum(w*erf(p/(sqrt(2)*sigma)))`. This is a condensed
Preisach population with history/remanence, not a memoryless tanh. Fresh p=0 is
the ideally demagnetized initial distribution. Set Ms from145mT remanence after
saturation, explicitly fitting that one major-loop scale, not minor-loop data.

Passivity: each row has positive derivative m'(p). With stored energy
`Psi(p)=integral_0^p u*m'(u)du`, actual movement has H-p=+/-r, hence
`integral H dm = deltaPsi + r*total_variation(m)`. Multiply by mu0 for J/m3;
closed loops dissipate nonnegative energy. Positive weights preserve this.
Use this proof plus independent explicit-relay/minor-loop/work tests.

Record current electronics: linear50us/5us lead shelf,0.010A/V gain; no invented
driver clipping. Playback: uniform-coating spacing/thickness/gap response,
Faraday induction plus leaky integrating EQ(30Hz), common fixed0.75V per unit
remanent normalized M, software outputVolts/8. This is a selected reduced flux-
equalized path, **not a claimed complete NAB50+3180us implementation**. Ideal
drivers omit actual current limits/core loss/circuit noise/offset. Measure the
resulting gain/flux/knee and head response rather than assert flatness from labels.

## One selected fault, before native implementation

Bias-only oscillator/output-buffer fundamental-current loss:
**Ib12mA peak healthy ->6mA peak selected fault**,100kHz unchanged.3mA severe
candidate retained in references only. Audio current drive/material/head/speed/
EQ/output gain are identical. This models one bias-buffer amplitude loss, not an
invented second audio resistor, DC imbalance, noise, motor effect or erase fault.

Pre-code128-samples/carrier/32-phase moving-parcel study,1kHz/.2V record input:
normalized H1/H3 healthy .274509/.00151369;6mA .525534/.0115237;3mA
.348670/.0482650. H3/H1~.55%/2.19%/13.84%. Half current increases sensitivity
and compression; severe candidate loses low-level response. Choose the moderate
case for intelligible comparison, no predetermined adjective. A slight extra
HF droop in this effective-depth model is **not proof of the classic brighter
underbias response**; depth/self-demagnetization and physical matching remain limits.

## Native candidate and pre-registered proof gates

First candidate: bias-phase expectation over8 phases, at most48 precomputed
bias-envelope extrema/endpoints per phase,8 row boundaries, fixed4x FIR audio
processing. Continuous full-carrier motion is independently resolved in long
double, never inferred from the native node outputs. Native averaging removes
the explicit carrier; retain finite-carrier spur/error evidence. Preliminary
long-double full-carrier768k output/128samples per carrier versus event-averaged
model, through common analytic playback/baseband filtering:1k/.2V RMS/peak
error .000520/.000658 normalized M;10k/.8V .000644/.001346. These are initial
healthy probes, not complete fault/rate validation or physical measurements.

Gaussian CDF: monotone Hermite table prepared off audio, positive derivative,
primitive error<=3e-7 against high precision. Full-carrier/phase/grid refinement,
explicit relay population refinement, positive/negative/remanence/minor-loop/
energy, two-tone/attacks/long note/chord/swept sine, DC/headroom, three rates and
alias/latency required. Ordinary material error RMS<=.001+.03*referenceRMS,
peak<=.003+.05*referencePeak; harsh RMS<=.005+.05*RMS,peak<=.02+.05*peak.
Replay head magnitude within.5dB of declared analytic response100..16kHz;
no solver fallback, numerical containment or PCM clipping in normal renders.
Refine numerical grids when a gate fails; do not relax gates to hide failure.

Standalone state<=16KiB, M7 object text<=24KiB, data/BSS0. Count actual fixed
work and measure full processor host time. First work bound:4*8*48*8=12288
row updates plus256 CDF evaluations/base sample and FIR/head work. This is
not a measured cycle count or future2000cycle/sample admission. No Daisy
performance/compatibility claim; retain the full-carrier reference if target
cost is excessive and propose approximation separately, without changing this sound.

Callbacks: fixed owned arrays, no allocation/free/IO/locks/JSON/throw/unbounded
solver; stopped transactional prepare/reset; visible invalid/clamp/failure counts.
Full Debug/Release, affected ASan/UBSan, existing TSAN, old golden/audio/source/
firmware preservation, actual pinned M7 layout/calls/stack audit. No integration.

## Founder source continuity / listening / elapsed metrics

Reuse exactly SF011's20s take03 mono PCM16 excerpt, SHA256
`75ae5e73c72e6774fb75e6318dc91ba66347fb65290949e947144033eb648f52`.
Verified against its retained CI archive, manifest and provenance. Original
stereo24, archive pin, nearest/ties-away conversion and full CC-BY4 attribution
remain. Founder prefers this recording; that is not approval of the new effect.
Low/medium/hard input groups use the same phrase at.05/.2/.8modelV peak,
identical healthy/fault input within each. Same raw software gain, pair-common
gain versions, separately labeled RMS-matched files with bounded disclosed gains.
Retain raw levels/crest/DC/physical currents/flux and any heavy matching. Controlled
synthetics remain separate. Human musical recognition/preference is pending.

First observed fetch01:10:16UTC; protected snapshot01:11:50; baseline completes
before this document. Record first WAV/checked-kernel/end gates in the report,
separately from untracked active labor. No hours-saved/96-circuit speedup claim.
