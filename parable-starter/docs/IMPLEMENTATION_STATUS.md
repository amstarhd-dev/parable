# Implementation status and reproducible baseline

## PAR-009 — prepared Seed3 first-boot kit, physical results pending

Started on main `0f84cb0` after #27 merged. [Prior-art/firmware Gate 0](PAR009_GATE0.md)
preceded source edits: unchanged Debug/Release **18/18** and pinned ARM baseline.
The same build now adds three BOOT_NONE images: independent onboard LED/recovery,
mono empty-patch bypass, and the **unchanged** Gain1.8/RAT.35/.5/.7/Lowpass1800,
trim.8 chain. Original four firmware binaries and three model objects match
baseline bytes. No kernel, canonical contract, correspondence or bootloader change.

Full Debug/Release **19/19**, affected ASan/UBSan **8/8**. New host simulations
cover clock/rate budget math, wrap/invalid intervals, bounded frozen publication
and status pulse patterns. Actual ARM links: LED **51,592 B**, instrumented bypass
**109,600 B**, RAT composite **109,784 B** load spans within 128 KiB. Capture state
**52 B**; accounting object has no external helpers/floating instructions. Audio
callback direct frame **64 B**, not measured cumulative/IRQ stack or runtime CPU.
The callback path contains actual RAT processing; no reporting or IRQ masking.

Only first-boot audio builds capture the first 30,000 callback bodies, freeze a
summary and publish once to control. USB CDC reporting begins afterward, using
existing pinned middleware and immutable text. Original rollback has no hooks.
SDK USB class allocation/IRQs and control-side send guard are outside capture;
their heap/latency/driver robustness still require hardware evidence.

[Founder procedure](SEED3_FIRST_DAY.md) covers USB-only ROM DFU/LED recovery,
the audited native Intel Mac utility candidate plus concrete Ubuntu fallback,
carrier rail/orientation/power gates and a pending-observation template. CI's
`seed3-first-boot-kit` keeps checksummed images/maps/logs/licenses/manifest and
rollback for 30 days. **No flashing, physical DFU/CDC or High Sierra execution,
audio/noise/headroom, callback deadline/CPU, heap/stack high-water or musical
approval has been tested.** No full-ISR/p99/latency inference from body counters.
Next: founder's standalone USB first-boot gate, then #26's verified carrier/power
plan before real-guitar audio and USB coexistence. No fourth effect/migration.

## PAR-008 — canonical RAT serial integration (software evidence)

Main `440a992` baseline passes 16/16 Debug/Release. [Gate 0](adr/ADR-001-par008-gate0.md)
cleared before source edits. The unchanged experimental LabRAT kernel now has an
explicit additive patch-v1 `Rat` revision 1 with three normalized controls, stable
IDs/order and strict one-instance/rate/parameter validation. Stopped preparation
and Engine loading preserve failure atomicity; disabled RAT does not advance state.
Eight utility stages/four RAM patch configurations remain; no live switching/storage.

Host suites pass **18/18 Debug/Release**, affected ASan/UBSan **7/7**. Four canonical
Engine synthetic WAVs include dry, RAT-only and both composite orders; standalone
RAT-only output matches exactly. Old RAT16/CMOS13/germanium25 render/report bytes
and standalone ARM objects match baseline. The actual active Engine+RAT Cortex-M7
image links in BOOT_NONE: **95,356 B load span**, **11,040 B Engine**; the bypass
startup target remains available with increased footprint. [Evidence/commands](PAR008_EVIDENCE.md)
record sections, state, static frames, expected full-codec overflow and warnings.
No hardware boot/run, CPU/deadline/latency/noise or real-guitar/founder approval.
Earlier sections below are historical; RAT's model/fidelity limitations still apply.

## SF-003 — positive-ground germanium fuzz specimen

Started on main `d628ffc3`, after PR #17 merged; clean Debug/Release **13/13** and
pinned ARM baseline passed. Proposed [ADR-005](adr/ADR-005-germanium-fuzz.md)
records source research before native code. One standalone PNP two-transistor
network adapts MIT ACME GP currents and published measured AC128 parameters,
including leakage, feedback, direct interstage coupling, BC caps and source loading.
Healthy plus Q1 BC-leakage and wrong-feedback-resistor experiments, without an
imposed4.5V bias. No new correspondence/sign, Engine, patch or hardware integration.

Fresh Debug/Release **16/16**, targeted ASan/UBSan **3/3**. Independent123 device
points,7bias cases and1344transient rows: DC peak17nV, jack72uV RMS/1.51mV peak;
192k grid improves all-node error over6x relative to96k. Twenty synthetic WAVs
provide matched and actual relative-level comparisons; normal renders zero
fallback/clipping. Existing RAT16-file regression and local CMOS13-render/report
bytes unchanged. ARM object **7108/0/0**, instance **2480 bytes**, not firmware-linked.
Prior model objects and firmware binary bytes unchanged; target CPU unmeasured.

Important limits: this untrimmed fitted sample predicts Q2-7.55V and about1.05k
input impedance, not common4.5V/5..8k targets. Coarse high-level1x diagnostic can
fail; accepted4x cases and alias fixture explicitly distinguish this. No new
physical-device calibration, real guitar, founder musical approval or hardware
feasibility claim. See [full report](GERMANIUM_REPORT.md) and
[actual reuse/work log](SOUNDFORGE_SF003_WORK_LOG.md).

## SF-002 — experimental CMOS inverter specimen

Started on main `2cc9412`, after PR #15 and its license-policy correction; clean
baseline Debug/Release **10/10** plus pinned ARM build passed. A separate portable
CD4049-derived two-inverter reduced circuit adapts MIT ACME device equations and
the published DAFx measured fit. Healthy controls and two component experiments
(33 nF -> 330 pF interstage coupling; 1 MOhm -> 100 kOhm second feedback), strict
configuration, bounded solver and 4x oversampling are implemented outside Engine.

Fresh host Debug/Release **13/13**, targeted ASan+UBSan **4/4**. Independent ngspice
checks: 362 DC points, 960 transient rows/five nodes; peak DC error 10.8 nV,
transient 0.815 mV RMS / 23.2 mV peak. Eight synthetic listening WAVs generated by
the same native kernel, explicit raw/final levels/configuration and zero fallback.
All sixteen LabRAT regression files and protected sources match unchanged.
ARM object **5604/0/0 text/data/BSS**, state **2128 bytes**, not linked into firmware.
Existing firmware binaries and RAT object bytes remain unchanged. No measured
target CPU/hardware, physical CD4049 calibration, real guitar, founder musical
approval, patch-runtime integration or new correspondence semantics.

See [research choice](adr/ADR-004-cmos-inverter-model.md), [full measured report](CMOS_REPORT.md)
and [work/reuse log](SOUNDFORGE_SF002_WORK_LOG.md). The ideal IC rail excludes the
original supply-dropper dynamics; gate-fit extrapolation remains unvalidated.

## SF-001 — host Soundforge bootstrap

Main after PR #13 is the measured starting point. Shared host-only statistics/dB/
RMS matching and unchanged PCM16 I/O, a three-asset offline evidence/provenance
registry, independent utility/negative registry tests, and a next-model recipe are
implemented. Fresh Debug/Release suites pass **10/10**; all sixteen original LabRAT
render/report files and protected DSP/reference hashes match the pre-edit baseline.
The pinned ARM build passes with unchanged object/instance sizes and firmware
binary bytes. No hardware, real-guitar or new musical acceptance is claimed.
See [SOUNDFORGE_BASELINE.md](SOUNDFORGE_BASELINE.md) and
[the host facility/recipe](../tools/soundforge/README.md). Productivity improvement
requires the next distinct effect; no universal solver or runtime integration added.

## PAR-007 LabRAT — circuit-derived processor and fault laboratory

Founder-supplied LabRAT brief supersedes the prior PAR-007 generic-drive integration
plan. Started from main `bb0635032bfae7b15a2e59de152040bf0a7dcb07`; PR #11 was
unmerged. Only its host PCM/synthetic/FFT lab pieces are reused, not PAR-006 DSP.
Required instructions and architecture read; baseline Debug/Release **4/4** each
passed before changes. See [source audit](LABRAT_SOURCE_AUDIT.md), proposed
[ADR-003](adr/ADR-003-labrat-circuit-model.md) and [full report](LABRAT_REPORT.md).

| Stage | Actual evidence |
| --- | --- |
| Source/audit/borrow | PiRAT/Rodent feedback polynomial/pot law port, exact source hashes, BSD/MIT notices; AudioLib source license absent, its code not imported; BYOD alternative audited |
| Native circuit-derived model | LM308/silicon RAT nominal reconstruction; explicit R/C/diodes, normal controls separate from circuit parameters, finite-A plus approximate rail/slew, coupled passive clipping/Filter/output model |
| Component variations | Positive diode disconnected; Ruetz feedback-leg opening; feedback compensation capacitor 100 pF -> 100 nF wrong-part experiment |
| Independent reduced-circuit reference | Actual ngspice 44.2 output: 333 complex preamp AC points and 576 diode/filter time points; numerical discrepancies recorded, not a full vendor LM308/JFET validation |
| Host-tested | Existing tests preserved; Debug/Release **6/6**; targeted ASan+UBSan **2/2**. Numerical/parameter/state/sample/block/DC/silence/slew/allocation and circuit-reference checks |
| Listening material | Eleven nine-second mono PCM16 synthetic dry/healthy/Filter/dynamics/fault WAVs, same C++ kernel, RMS matched; passing-CI download plus exact settings and measurements |
| Human musical acceptance | **Pending founder listening/review** |
| ARM | Actual Cortex-M7 object 6,452/0/0 bytes text/data/BSS, ARM state 10,752 bytes; no linked/activated RAT firmware or target execution |
| Hardware / real guitar / pedal-grade | **Not demonstrated** |
| Specific historical revision / full component fidelity | **Not authenticated**; reduced model/idealized buffers and overload dynamics have material limits |

Independent preamp agreement is about 9e-7 dB after bilinear frequency alignment;
actual same-frequency warping reaches 0.324 dB. Reduced clipping/Filter waveform
error is about 0.50 mV RMS / 0.91 mV peak. Selected 1x/4x alias fixture improves
35.92 dB, with residual -50.58 dBc. These figures are model/fixture evidence,
not physical-pedal fidelity or a universal alias bound. Diode opening charges the
raw node to about +3.70 V in the test and reduces steady clipping; ideal output
buffer limitations make large-fault post-buffer fidelity uncertain.

No astrology rules, closed patch v1, native Engine/preparation or controls/storage/
activation architecture changed. No generic drive is silently registered. No
callback allocations, I/O, locks, throws or iterative circuit solver; preparation
is stopped-control-side only. ARM evidence includes exact object/layout/work
estimates, not measured CPU/latency/noise/power or stack high-water.

Unfulfilled evidence is explicit: blocked ElectroSmash access, no verified factory
year/BOM or manufacturer LM308/JFET model/measurements, no real guitar audition,
no hardware/Seed3 boot or musical approval. Source borrowing and circuit-equation
validation are substantive progress, not grounds to declare the whole RAT faithful.
Next: founder audition and an agreed factory/reference capture, then refine the
missing component dynamics before permanent patch integration. Overall project
licensing remains an owner decision; borrowed notices are retained.

## PAR-005 ARM toolchain/resource gate — 2026-11-08

Started from main `849143b0886eb9de0cc6eceba7d5d9c6345df53c`, after PR #7 merged.
Read the agent rules, constitution, current status and firmware integration first.
Unchanged host Debug and Release suites each passed **4/4** before edits. Detailed
pins, commands, audit, metrics, warnings and bring-up requirements are in
[ARM_BUILD_BASELINE.md](ARM_BUILD_BASELINE.md); exact dependency/license records
are in [ARM_DEPENDENCIES.md](../third_party/ARM_DEPENDENCIES.md).

| Component/evidence | Host | ARM cross-build/link | Physical hardware | Musical audition |
| --- | --- | --- | --- | --- |
| Native DSP core | Existing tests preserved | In actual Cortex-M7 hard-float minimal ELF | Not flashed/run | Not auditioned |
| Native preparation adapter | Existing host tests preserved | ARM object, text 8,380 bytes; also linked into SRAM/QSPI diagnostics | Not run | Not auditioned |
| JSON codec/control startup | Existing host tests preserved | Internal FLASH diagnostic overflows by 65,380 bytes; same diagnostic links with official BOOT_SRAM and BOOT_QSPI | Not run | Not auditioned |
| Minimal Seed3-oriented callback | Unchanged portable processing behavior | ELF/bin/map generated; no JSON symbols; configured-rate query compiled | Codec/PLL/DMA/analog path unverified | Not auditioned |
| Live activation, flash presets, compiler, UI | Unchanged/deferred | Not implemented by this gate | Untested | Untested |

Selected xPack GNU Arm Embedded GCC 14.2.1-1.1 is archive-checksummed; libDaisy
`1289a000bc29dae1bc0ed4129cfa2df40ef71574` and five required driver submodules
are commit-pinned/verified. Cortex-M7 Thumb/FPv5-D16 hard float, release `NDEBUG`,
GNU C++17 application. Internal-flash **BOOT_NONE**, linker budget 128 KiB at
`0x08000000`; no Daisy software bootloader deployed. The module's 8 MB external
QSPI and 64 MB external SDRAM are not treated as this internal image budget.
The recipe also builds fuller startup JSON decode/prepare diagnostics with the
unmodified official SRAM and QSPI linkers: each bin/load span is **196,292 bytes**,
Berkeley text/data/BSS **194,148 / 2,092 / 28,992**. SRAM executes in AXI with
194,912 bytes of executable VMA span (196,292-byte load span including `.data`
initializers); ordinary data/BSS span 13,980 bytes in DTCM. QSPI executes at
`0x90040000`, ordinary data/BSS span 13,980 bytes in AXI. Both use 16,384 bytes D2
DMA and 12 bytes backup SRAM, no static external SDRAM. These exclude loader
resource use and dynamic heap/stack. Direct main frame is 328 bytes; callback 20.

Actual minimal release ELF Berkeley text/data/BSS: **72,392 / 1,616 / 28,608**.
Flash load span/bin: **74,008** bytes. AXI static address span: **13,124** bytes
(13,120 section-byte sum); D2 DMA: **16,384**; backup SRAM: **12**; static external
SDRAM/QSPI: **0**. Compiler direct frames: callback **20**, main **120** bytes.
Heap/stack high-water and call-chain/IRQ stack cost remain **not measured**.
The report explicitly separates vector/data placement and coalesced text/rodata;
ELF debug-file size is not confused with deployed flash usage.

Reproduce from repository root:

```bash
python3 parable-starter/firmware/build.py
```

A fresh dependency checkout and a second cache path both reproduced the minimal
binary hash recorded in the detailed baseline. The **65,380-byte FLASH overflow**
is a BOOT_NONE layout limitation, not proof JSON exceeds Daisy's total capacity.
The script requires all three candidate images to link and treats only the
internal-flash diagnostic's identified overflow as non-gating;
unexpected API/compile/link failures fail. CI retains logs, ELF/bin/map, section/
symbol/disassembly reports, input hashes and notices; exact run links are in PR.

Firmware now constructs a fixed-storage Engine after `DaisySeed::Init`, using
`AudioSampleRate()` and checking load before starting the callback. That API
reports configured rate, not a hardware clock measurement. The pinned SDK uses
48 kHz/24-bit SAI on the shared Seed3/TAC5242 path; advertised 192 kHz/32-bit
capability is not demonstrated. Mono input 0 still feeds both outputs. No callback
allocation, parsing, locks or I/O added, and fast/finite-math flags are rejected.

Unresolved: upstream startup FP-handler warning, unsupported nosys syscall warnings
and RWX ELF segment warning are retained, not suppressed. Runtime clocks, SDRAM,
CPU overhead, heap/stack/RAM headroom, latency/noise, power/heat, electrical safety,
guitar impedance/level handling and sound remain **unmeasured/untested**. Actual
bring-up requires a verified module/dev kit, flashing/recovery plan, instrument-
suitable analog front end and measurements. No direct passive-guitar connection
or target performance claim. JSON footprint options need separate review; no
new format, enlarged linker budget, effects or correspondence rules were added.
The bundled v6.4 intdfu/2000ms loader binary is hash-pinned/audited, not rebuilt,
flashed or executed. Software layout/QSPI configuration aligns with the pinned
Seed3-aware SDK, but official loader README does not explicitly name Seed3:
**Seed3 boot compatibility remains unverified**. See the detailed audit and
SRAM-versus-QSPI timing/RAM tradeoffs. Linked diagnostics' newlib `_sbrk` has no
region bound; dynamic allocation/exhaustion safety remains unproven.

The planned ESP32-S3 may own JSON/control processing while Daisy receives a
compact prepared plan. Native Engine loading has no JSON dependency. Canonical
JSON authority need not change; a bounded, versioned transport, rate/capability
negotiation, DSP-side validation and storage/activation ownership require a
separate decision. No ESP32 code, wire format or functionality cut was added.

Next gates: measured on-hardware signal path, separately specified live patch
ownership, and an actual correspondence-to-audio trace. These do not replace the
correspondence purpose with a hardware-only roadmap. Earlier sections below are
historical; PAR-005 supersedes only their previously-unproven ARM build evidence.

## PAR-004 stopped-audio host preparation — 2026-11-08

Started from latest main `4f44ca25897cee79dad068302ac612944586c91d`, including
merged PR #5. Read the required instructions, ADR, format, headers and existing
tests first. Both unchanged baseline Debug and Release suites passed **3/3**
before edits. This is the first decode -> prepare -> stopped Engine load -> mono
host playback slice, limited to existing Bypass/Gain/OnePoleLowpass plumbing.

`preparation::prepare(const patch::ValidatedPatch&, float actualSampleRate)`
returns an owned `parable::Patch` or contextual Error. It resolves IDs through
`serialOrder()`, copies flags/settings/trim, requires exact rate equality and
revalidates with `parable::validate` at the actual rate. No parameter clamping,
resampling, silent omission or reordering. Bypass's unused native parameter is
explicitly 1. Native `schema=2` remains internal; the JSON format remains version 1.
The adapter never loads an Engine and retains no pointers/references in its result.

| Capability | Source | Host tests | Daisy build | Hardware | Musical audition |
| --- | --- | --- | --- | --- | --- |
| Typed document -> ordered native patch | Implemented | All 15 accepted/golden documents; exact stage fields/trim | Not built | Untested | Not auditioned |
| Mono stopped-audio load/render | Existing Engine plus new bridge | Independently hand-built native patches/Engines; long filter history and finite bounds | Not built | Untested | Not auditioned |
| Target-rate/error isolation | Implemented | Invalid/mismatched rates, native validation and guard faults; old patch/history/document unchanged | Not built | Untested | Not auditioned |
| Live activation, flash, compiler, UI, new effects | Not implemented | Outside scope | Not built | Untested | Not auditioned |

Changed files and reasons (relative to `parable-starter/`):

- `include/parable_prepare.h`: small public control-side Result/Patch/Error API.
- `src/parable_prepare.cpp`: ID resolution, mapping and actual-rate native validation.
- `src/prepare_internal.h`: private borrowed source view for testing guards without
  weakening ValidatedPatch. Not a second public input/patch authority.
- `tests/test_prepare.cpp`: independent native expectations for every accepted
  fixture; rendering, order, flags, ownership, capacity/rates and failure isolation.
- `CMakeLists.txt`: JSON-independent bridge library and `host_preparation` test.
  The existing Debug/Release CI matrix runs it without workflow changes.
- `README.md`, `docs/PATCH_FORMAT_V1.md`, this status report: current API boundary,
  unchanged wire rules, evidence and limitations.

Exact reproduction from repository root (fresh build directories):

```bash
cmake -S parable-starter -B build-par004-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-par004-Debug
ctest --test-dir build-par004-Debug --output-on-failure --no-tests=error -V
cmake -S parable-starter -B build-par004-Release -DCMAKE_BUILD_TYPE=Release
cmake --build build-par004-Release
ctest --test-dir build-par004-Release --output-on-failure --no-tests=error -V
```

Host evidence: Linux x86_64, GNU C++ 14.2.0, CMake/CTest 3.31.6. Final local and
GitHub CI run links and exact commands are recorded in the PR. Local Debug and
Release passed **4/4**, including 658,386 new host-preparation checks (many repeated
sample invariants). An AddressSanitizer/UndefinedBehaviorSanitizer Debug build
also passed 4/4 with no diagnostics; no compiler warnings were emitted.
Baseline and new checks remain active in Release. No existing DSP algorithm,
processor library, JSON contract/codec, fixture or correspondence semantics changed.

Tests inspect mapped stages directly so commuting small-signal gain/filter audio
cannot hide wrong order. A separate two-gain fixture with unchanged IDs produces
0.64 vs 1.0 on input 200 using the **existing** +/-256 internal saturation rail;
small signals commute. This overload example is not an audible-order/fidelity
claim. All accepted fixtures render 4096 ordinary samples, explicit extremes,
then 4096 recovery samples against independent expected-patch Engines.

Failure tests preserve both Engine configuration and its evolving filter history.
Malformed private source views exercise excessive counts, unresolvable/zero IDs,
duplicate IDs/order, invalid gain/cutoff/trim/rate and native-status propagation.
These states cannot be constructed through the current public ValidatedPatch API.
Unsupported JSON types reject at decode; the closed current variant has only
three supported alternatives. The adapter's future unsupported/valueless-settings
fallback is defensive and has no constructible runtime case tested here. No
const-cast, forged ValidatedPatch or public validation bypass was introduced.

Unproven/incomplete: Daisy compilation/flashing, actual hardware-rate query,
CPU overhead, heap/stack/target RAM, latency/noise, real guitar and musical quality
remain **unbuilt/unmeasured/untested**. The caller supplies the actual rate and
must use it consistently: a returned mutable prototype Patch has no rate/ID tags.
Native stages are execution copies; retain the validated document as authority.
Error strings can allocate/throw on resource exhaustion on the control side.
Neither parsing nor preparation belongs in audio. Engine::load still requires
stopped audio; no ownership protocol, atomic swap, clickless transition, persistent
storage or final immutable runtime plan is supplied. Next: independently review
this boundary, obtain target toolchain/resource evidence, and specify any live
activation as a separate ticket with its own concurrency/hardware gates.

## PAR-002 control-side patch contract — 2026-11-08

This section records the earlier contract-only increment; PAR-004 above adds the
temporary stopped-audio bridge, while final plans/activation/storage stay deferred.

Started from main `026bc5df1833a2662e40c99e483f9c43b1758433`, after PAR-003
merged. The existing Debug suite passed 2/2 before code changes. Read the agent
rules, constitution, this report, ADR-001 and core header first. Founder direction
selects ADR-001 Option A; the separately reviewable decision amendment records
that choice without claiming activation or storage implementation.

Implemented: `parable.patch` version 1, a bounded strict JSON reader, field/unit/
number/serial-graph validation, fixed-capacity read-only typed results, canonical
serialization, golden and malformed fixtures, synthetic creator parity and four
independent RAM-held serialized payloads. See [PATCH_FORMAT_V1.md](PATCH_FORMAT_V1.md)
for the normative contract and [third-party provenance](../third_party/README.md).
This is a separate control-side static library; `include/parable_core.h`, existing
audio tests and firmware are unchanged. No new code runs in audio. There is no
conversion to an active/prepared DSP plan and no persistent bank-file format.

| Capability | Source code | Host tests | Daisy build | Actual hardware | Musical audition |
| --- | --- | --- | --- | --- | --- |
| Versioned JSON, explicit typed parameters/units, stable IDs | Implemented | Debug/Release | Not built | Untested | Not auditioned |
| Serial-only graph validation and field/rule errors | Implemented | Valid/invalid fixtures and limits | Not built | Untested | Not auditioned |
| Canonical binary32 round trips | Implemented | Golden bytes, halfway decimals, 4096 generated numeric cases | Not built | Untested | Not auditioned |
| Manual/chart-like creator parity | Synthetic fixture/demo functions only | Equal validated data and canonical bytes | Not built | Untested | Not auditioned |
| Four independent serialized payloads | Four host RAM strings | Isolation and encode/decode parity | Not built | Untested | Not auditioned |
| Runtime plan/activation, flash storage, real correspondence compiler, UI | Deferred | Not implemented/tested | Not built | Untested | Not auditioned |

Changes: new public `include/parable_patch.h`; private `src/patch_json.{h,cpp}`
ingress and `src/parable_patch.cpp` validation/serialization; new `tests/test_patch.cpp`
and `tests/fixtures/patch-v1/`; control-library/test CMake targets; format and
dependency documentation/license; README/status updates; ADR decision amendment.
The existing Debug/Release CI matrix discovers all three tests without a workflow
change. No effects, correspondence semantics, callback, activation or storage work.

Exact host commands from repository root, using separate initially absent build
directories and CMake/CTest 3.31.6 with GNU C++ 14.2.0 on Linux x86_64:

```bash
cmake -S parable-starter -B build-par002-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-par002-Debug
ctest --test-dir build-par002-Debug --output-on-failure --no-tests=error -V
cmake -S parable-starter -B build-par002-Release -DCMAKE_BUILD_TYPE=Release
cmake --build build-par002-Release
ctest --test-dir build-par002-Release --output-on-failure --no-tests=error -V
```

Observed: clean configure/build and 3/3 tests passed in each mode, no compiler
warnings. Checks remain active under Release/NDEBUG. Additional Debug build with
`-DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer'` also
passed all three tests without sanitizer reports. GitHub run links and final
results are recorded in the PR; local results are not substituted for remote CI.

Evidence: 45 checked-in JSON fixtures (15 accepted/golden documents and 30 malformed
documents); 4096 seeded binary32 cases; exact field/code assertions on rejection;
duplicate escaped keys, unsupported versions/types/units/fields, missing/extra
parameters, byte/depth/value/string/processor/edge limits, invalid endpoints,
branches, merges/duplicate edges, feedback and disconnected cycles. Reordering
preserves same-type instance IDs/settings while changing explicit signal order.
Failed decode returns a separate Error, leaving existing validated/canonical data
unchanged. Two synthetic creators use the same public API; no astrology rules
are present. Four serialized strings are independent; no nonvolatile claim.

Limitations and unresolved future design work:

- Version 1 intentionally rejects provenance, metadata, expression mappings and
  unsupported processor revisions. Their semantics and version migrations need
  separate decisions; unknown fields are never silently dropped.
- The sample rate is explicit validation context. Future preparation must check
  the actual device rate/resource budget; automatic conversion is not specified.
- IDs are stable patch-local uint32 values; cross-document editing/ID-allocation
  policy belongs to later composer work. No sharing/inheritance between slots.
- Limits (8 processors / 16 KiB JSON) and nlohmann DOM use are unmeasured on MCU.
  ARM C++17 floating `charconv`, exceptions/allocation support and memory budget
  may require a smaller parser/toolchain work. That need does not authorize a
  second playback authority. Cross-language codec parity is not yet tested.
- A pinned dependency download is required for a fresh host build; verified
  offline-source override is documented. Host ingress allocates and uses
  exceptions; resource exhaustion can propagate without modifying prior values.
- Daisy firmware, CPU overhead, heap/stack/target memory, hardware, real guitar,
  analog clipping and musical quality remain **unbuilt/unmeasured/untested**.
  Sanitizer success is host evidence only; no exhaustive/fuzz campaign is claimed.

Next smallest work: independently review/merge this data-contract increment,
then assess the actual Daisy toolchain/memory cost before a separate runtime-plan
or ownership-exchange ticket. Activation and flash journaling remain independent
future work, not implied by successful JSON round trips.

## PAR-003 numerical safety update — 2026-11-08

Work started from latest main `9111f8799b5b09ff19ef9e38e21f1ae233c6321b`.
Read `AGENTS.md`, `PRODUCT_CONSTITUTION.md` and this report before editing code.
The original Debug smoke suite passed (1/1) before changes. This update addresses
[issue #3](https://github.com/amstarhd-dev/parable/issues/3) only; it does not
implement the patch ADR, effects, persistence or correspondence compilation.

Current safety policy in `Engine::process`:

- NaN and both infinities return zero without advancing filter history.
- Finite input, each gain result and each filter history value saturate at
  +/-256 internally. This is 2^8, retaining the existing headroom of eight gain-2
  stages for ordinary input in [-1,1]. Filter histories start at zero, so both
  operands of the filter difference stay within +/-256 and the difference within
  +/-512. Valid coefficients are finite and between zero and one; gain is at
  most 2. Intermediates therefore stay far below float overflow. Existing final
  output clamping remains [-1,1]; trim/volume zero yields zero without Inf * 0.
- The rail may alter extreme overloads followed by attenuation or filter tails.
  It is numerical emergency saturation, not analog modeling, a complete limiter,
  or a promise of musical quality. Revisit its headroom if stage capacity or
  processor gain rules change. The callback still uses float arithmetic, bounded
  loops and fixed arrays; no allocations, exceptions, I/O or locks were added.

Changed files and reasons:

| File (relative to `parable-starter`, except workflow) | Change |
| --- | --- |
| `include/parable_core.h` | Bound signal and filter history before they can overflow; retain final clamp and rejected-input policy. |
| `tests/test_check.h`, `tests/test_core.cpp` | Replace disappearing `assert` setup/checks with single-evaluation, always-active checks and failure exit status; retain original checks. |
| `tests/test_audio_safety.cpp` | Add original overflow regression, adversarial sequences, filter/rejected-input/rejected-patch recovery, ordinary-input reference and seeded invariant coverage. |
| `CMakeLists.txt` | Register the separate `audio_safety` host executable/test. |
| Repository-root `.github/workflows/host-tests.yml` | Run a Debug/Release matrix; Release keeps the normal `NDEBUG` flag. |
| `README.md`, `docs/IMPLEMENTATION_STATUS.md` | Update test commands, safety policy, evidence and limitations. |

Observed host evidence: GNU C++ 14.2.0, Linux x86_64, CMake/CTest 3.31.6.
With new tests and the **unchanged original engine**, Debug and Release both
built successfully but CTest exited 8: `core_validation` passed and `audio_safety`
failed at `std::isfinite(sample)` on the original eight-gain/zero-trim reproducer.
Release flags included `-O3 -DNDEBUG`; its checks did not disappear.
After the fix, fresh Debug and Release build directories both passed **2/2 tests**
without warnings. Both execute 23 original core checks and 1,476,661 audio-safety
checks. These counts include repeated invariants, not that many distinct cases.
GitHub Actions run links and outcomes are recorded in the focused PR description.

Reproduce final builds from the repository root (separate, initially absent
build directories; use installed CMake or the environment-only setup below):

```bash
cmake -S parable-starter -B build-par003-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-par003-Debug
ctest --test-dir build-par003-Debug --output-on-failure --no-tests=error -V
cmake -S parable-starter -B build-par003-Release -DCMAKE_BUILD_TYPE=Release
cmake --build build-par003-Release
ctest --test-dir build-par003-Release --output-on-failure --no-tests=error -V
```

Reproduce the expected pre-fix failure with the final regression source (each
test binary below must exit 1 at the finite-output check):

```bash
mkdir -p /tmp/parable-par003-before
git show 9111f8799b5b09ff19ef9e38e21f1ae233c6321b:parable-starter/include/parable_core.h > /tmp/parable-par003-before/parable_core.h
c++ -std=c++17 -O0 -g -I /tmp/parable-par003-before parable-starter/tests/test_audio_safety.cpp -o /tmp/parable-par003-before/debug
/tmp/parable-par003-before/debug
c++ -std=c++17 -O3 -DNDEBUG -I /tmp/parable-par003-before parable-starter/tests/test_audio_safety.cpp -o /tmp/parable-par003-before/release
/tmp/parable-par003-before/release
```

Coverage includes positive/negative maximum finite floats, repeated alternating
extremes, zero/negative zero, subnormals, NaN/both infinities, zero trim/volume,
filter decay/recovery at 8/48/192 kHz and cutoff bounds, and unchanged signal/state
after rejected patches. Ordinary [-1,1] bypass/gain/filter/mixed/disabled-stage
sequences agree with an independent double-precision, unsaturated reference
within absolute tolerance **2e-5**. A fixed seed generates 128 valid patches and
1024 binary32 bit-pattern inputs per patch; each output is checked finite and
within [-1,1], followed by zero-input recovery checks. This is deterministic
sampling plus explicit edge cases, not exhaustive enumeration or musical proof.

**Untested / not measured:** Daisy firmware compilation/flashing, actual callback
CPU overhead, heap/stack/target memory, real guitar, analog clipping, listening
quality and hardware timing. No fast-math build was tested; this safety policy
requires ordinary IEEE-aware floating-point compilation. Existing stopped-audio
loading/concurrency restrictions remain. There is no new hardware or musical
completion claim. Next: independently review this safety fix; broader PAR-002
codec work starts from updated main only after this PR is reviewed and merged.

## PAR-001 historical baseline

The remainder preserves the original audit at its inspected commit. Its known
overflow defect and assertion-only testing descriptions are superseded by the
PAR-003 update above; its untested product/hardware limitations still apply.

Recorded: 2026-11-08. Inspected baseline commit:
`a47396d6037e33d635d128f42dc6c9c7c1718273` in `amstarhd-dev/parable`.
This is an engineering baseline and proposed decision, not release readiness.

## Inventory and instructions

All 11 files tracked at the inspected commit were read. Paths below are relative
to the Git repository root; the CMake project lives inside `parable-starter/`.

| Tracked file | Role and limits |
| --- | --- |
| `README.md` | Two-line repository description; no build instructions. |
| `parable-starter/AGENTS.md` | Agent/process rules; requires a baseline before code changes and a proposed two-option ADR. |
| `parable-starter/PRODUCT_CONSTITUTION.md` | Proposed product purpose, pilot scope, gates and founder decision rights; specification, not completion evidence. |
| `parable-starter/README.md` | Brick 0.1 build instructions and explicit prototype limitations. |
| `parable-starter/docs/CODEX_KICKOFF.md` | Initial task and historical status; incorrectly claimed a CI file was present. |
| `parable-starter/docs/HARDWARE_SUPPLY_DECISION.md` | Dated Seed3/carrier/supply proposal; no hardware measurements. Its vendor/stock/price assertions were not reverified in PAR-001. |
| `parable-starter/CMakeLists.txt` | C++17 host executable, warning flags, and the sole CTest entry `core_validation`; does not build firmware. |
| `parable-starter/include/parable_core.h` | Entire header-only portable core: types, validation, four-slot RAM bank, stopped-audio loading, mono gain/filter processing, volume and clamp. |
| `parable-starter/tests/test_core.cpp` | Only automated test source: one assert-based smoke executable. Coverage is detailed below. |
| `parable-starter/firmware/daisy_pass_through.cpp` | Uncompiled libDaisy adapter: hardcoded 48 kHz engine, default hardware initialization, left input processed and duplicated to both outputs; right input unused. |
| `parable-starter/firmware/Makefile.example` | Integration example requiring separately built libDaisy and ARM tooling; not a working project build target in this checkout. |

The four requested instruction files are present under `parable-starter/`;
none was fabricated or replaced. There are no nested agent instructions.
The claimed `.github/workflows/host-tests.yml` was **missing**, and no workflow
was tracked anywhere in the baseline. GitHub's Actions runs endpoint returned
`total_count: 0` before changes. No existing CI failure was available to diagnose.
The complete astrological correspondence rules, historical browser v3.3 and
Python renderer are not tracked here. Their correctness cannot be verified from
this source set; no such rules or historical implementations were introduced.

Instruction alignment findings:

- `AGENTS.md` calls the product an embedded guitar-effects product; the
  constitution already centers correspondence. PAR-001 proposes constitutional
  clarification that effects DSP enables correspondence, and keeps this wording
  discrepancy visible rather than rewriting the agent rules.
- Snapshot immutability, atomic activation and safe levels in the rules are
  requirements. Current copied structs and stopped-audio `load` only partially
  satisfy them; the prototype is not a concurrent patch authority implementation.
- The kickoff's suggested ADR filename is superseded by the user's requested
  `docs/adr/ADR-001-patch-authority.md`; its link is updated, without a second ADR.
- The hardware supply document describes a future carrier more definitively
  than the current physical-form investigation. Treat that as a dated proposal;
  carrier, guitar mounting and manufacturing form remain unvalidated. No board
  or mounting assumptions were added to the audio architecture.

## Build and test evidence

A fresh shallow clone of the private repository was made at the baseline commit.
`git status --short` was empty before configure. No tracked source changes were
made before the initial build and tests, and results were reported before edits.

Host: Linux x86_64, GNU C++ 14.2.0, CMake/CTest 3.31.6, Unix Makefiles.
CMake was initially absent. Environment-only setup (not a project dependency):

```bash
python3 -m venv /workspace/work/baseline-tools
/workspace/work/baseline-tools/bin/python -m pip install 'cmake==3.31.6'
export PATH=/workspace/work/baseline-tools/bin:$PATH
```

Reproduce from a clean checkout with Git, CMake >= 3.15, Make and a C++17 compiler:

```bash
git clone https://github.com/amstarhd-dev/parable.git
cd parable
git checkout a47396d6037e33d635d128f42dc6c9c7c1718273
cd parable-starter
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Private repository authentication is required. To verify the PR revision instead,
check out its commit before configuring; no root-level CMake project exists.
Debug matters: tests use `assert` for checks **and setup calls**. `NDEBUG` in a
Release build removes both and can yield a misleading pass. Baseline compiler
flags were `-g -std=gnu++17 -Wall -Wextra -Wpedantic`, with no `NDEBUG`.

Actual initial results (all three commands exited 0):

```text
-- The CXX compiler identification is GNU 14.2.0
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features - done
-- Configuring done (0.3s)
-- Generating done (0.0s)
[ 50%] Building CXX object CMakeFiles/parable_core_test.dir/tests/test_core.cpp.o
[100%] Linking CXX executable parable_core_test
[100%] Built target parable_core_test
    Start 1: core_validation
1/1 Test #1: core_validation ..................   Passed    0.00 sec
100% tests passed, 0 tests failed out of 1
Total Test time (real) =   0.00 sec
```

No compiler warnings were emitted. This is the entire existing automated suite,
not a firmware build or a musical test. The test executable prints
`PARABLE_CORE_V0_1: host validation passed`.

## What the existing test actually exercises

- All four slots initialize empty. Setting slot 0 leaves slot 1 empty, and
  mutating the source afterward leaves the saved stage value unchanged. It does
  not write/recall four distinct patches or exercise storage across restarts.
- An invalid gain edit returns `InvalidParameter` and preserves the prior gain.
  Wrong schema and unknown processor type reject. Selecting slot 4 rejects;
  selecting slot 0 succeeds. `Bank::get` itself has no bounds check.
- A gain/low-pass chain produces a finite positive first sample in a loose range.
  Swapping those linear processors gives an approximately equal first sample.
  Despite an earlier comment about stage order, this is **not** proof that
  noncommuting routing or meaningful topology changes work.
- Empty-patch dry processing, a scalar volume setting, output clipping on input
  9.0, and NaN input returning zero are checked. Filter response, long-run
  stability, aliasing, level matching and musical quality are not checked.
- `Engine::load` rejects an unknown type. The test does not assert preservation
  of all runtime state after rejected loads or exercise any concurrent operation.

## Functionality by evidence stage

“Specified” means a documented requirement; “implemented” means code exists.
“Host-tested” refers only to observed checks, not all possible behavior.
Hardware-tested and musically auditioned are independent evidence stages.

| Functionality | Specified | Implemented | Host-tested | Hardware-tested | Musically auditioned |
| --- | --- | --- | --- | --- | --- |
| Versioned canonical patch authority | Yes | Internal `schema=2` struct only; ADR proposed | Schema mismatch rejection only | No | No |
| Processor instances and routing | Yes | Up to 8 ordered stages; no stable IDs or edges | Two-stage linear-chain smoke only | No | No |
| Parameters and units | Yes | Per-type anonymous float and bounds; sample-rate-dependent cutoff | Invalid gain/type/schema cases only; not full boundary/NaN matrix | No | No |
| Four independent presets | Yes | Four RAM values and selected index; selection does not load Engine | Defaults/source-copy isolation, slot 0 vs 1, select bounds only | No | No |
| Durable save/load and round trips | Yes | No codec, flash journal or persistence; image-status enum values are unused | No | No | No |
| Immutable runtime snapshot / live activation | Yes | Engine copies a patch while audio is stopped; no exchange protocol | Synchronous load smoke only | No | No |
| Gain / one-pole low-pass / dry path | Utility plumbing | Yes, mono; hard output clamp | First-sample chain and dry/volume/clamp/NaN smoke | No | No |
| Register, master Volume, performance bypass | Yes | Scalar engine volume and empty-patch dry path only; no Register or physical controls | Basic volume/dry checks only | No | No |
| Safe output under all accepted inputs | Yes | Finite-input check and final clamp, with a known overflow gap below | Limited smoke passes; diagnostic gap reproduced | No | No |
| Traditional Western astrology compiler/trace | Yes | None | No | No | No |
| Phone-first Workshop composition/editing | Yes | None | No | No | No |
| Biometric and other grammars | Future extension | None | No | No | No |
| Standalone saved playback | Yes | Uncompiled startup-bypass adapter; no saved patch loading | No | No | No |
| Daisy Seed3 integration | Initial target | Demonstration adapter only; no verified toolchain/sample-rate setup | Not built by host CMake | No | No |
| Circuit-modeled effects / musical environments | Pilot requirements | None; gain/filter are plumbing | No musical/model validation | No | No |

No functionality is hardware-proven, auditioned or release-ready by this audit.
CPU load, worst-case callback time, heap/stack use, target memory use, latency,
noise floor, power consumption and switching transients: **not measured**.
Fixed array capacities seen in source do not establish measured target budgets.
The callback has no obvious allocating/I/O calls in source; no allocation or
real-time instrumentation was run.

## Minimal CI and baseline changes

| Changed file (repository-relative) | Reason |
| --- | --- |
| `.github/workflows/host-tests.yml` | Add the missing host workflow where GitHub discovers it; checkout, run in `parable-starter`, configure Debug, build, run CTest with an error if no tests are found. Push and PR triggers, read-only contents permission, five-minute timeout. |
| `parable-starter/.gitignore` | Ignore the documented local `/build/` output so the baseline remains reviewable. |
| `parable-starter/README.md` | Specify correct working directory, Debug/assert requirement, and links to evidence and ADR. |
| `parable-starter/docs/CODEX_KICKOFF.md` | Correct missing-CI history and link to the requested ADR location. |
| `parable-starter/PRODUCT_CONSTITUTION.md` | Proposed minimal purpose/phone/standalone/future-grammar/form-factor clarifications, explicitly subject to founder review. |
| `parable-starter/docs/IMPLEMENTATION_STATUS.md` | Inventory, reproducible evidence, stage separation, known gaps and next ticket. |
| `parable-starter/docs/adr/ADR-001-patch-authority.md` | Exactly two concrete alternatives, recommendation and implementation gates; no architecture implemented. |

No C++ source, tests, CMake targets, effect topology or astrological rule changed.
CI execution evidence for the PR is recorded in its description/checks, including
run links and any failures; a local pass is not described as a GitHub Actions pass.

## Unresolved risks and missing tests

- **Non-finite output from finite input:** an additional disposable host diagnostic
  found that a valid eight-stage gain patch (gain 2 each), `outputTrim=0`, and
  `std::numeric_limits<float>::max()` input overflows internally; infinity times
  zero becomes NaN, which the final comparison-based clamp does not remove.
  Observed output: `load_ok=1 output_is_finite=0`. This contradicts the source
  comment claiming always-bounded output. It is not a claim that normal guitar
  input reproduces it. Source was left unchanged to keep this baseline/proposal
  focused; output safety needs a small dedicated regression/fix before expanded
  DSP or hardware acceptance. Reproduce from repository root:

  ```bash
  c++ -std=c++17 -I parable-starter/include -x c++ -o /tmp/parable-finite-probe - <<'CPP'
  #include "parable_core.h"
  #include <cmath>
  #include <iostream>
  #include <limits>
  int main() {
      parable::Patch p;
      p.count = parable::kMaxStages;
      for (auto& s : p.stages) s = {parable::Type::Gain, 2.f, true};
      p.outputTrim = 0.f;
      parable::Engine e(48000.f);
      const auto status = e.load(p);
      const auto y = e.process(std::numeric_limits<float>::max());
      std::cout << "load_ok=" << (status == parable::Status::Ok)
                << " output_is_finite=" << std::isfinite(y) << '\n';
  }
  CPP
  /tmp/parable-finite-probe
  ```

- **Concurrency:** `Bank::set` is validate-then-copy, not a thread-safe atomic
  operation. `Engine::load`, `setVolume`, and `Bank::get` cannot be raced with
  their readers. The engine's copied configuration is private, but immutable
  snapshot lifetime/reclamation is not implemented. No race tests were run.
- **Validation:** no exhaustive bounds/type/sample-rate/count tests, parser
  fuzzing or serialization corrupt-image/version tests exist. `Bank::get` needs
  a valid index; the low-pass header's 20..20000 Hz comment differs from actual
  validation (< 0.45 * sample rate). Neither gap was silently corrected here.
- **Firmware:** not compiled, linked, flashed or connected to physical audio.
  libDaisy/toolchain version compatibility and actual sample rate are unverified.
  No physical switches, analog front end, supply behavior or mounting evaluated.
- **Persistence and activation:** no power-cut, storage wear, four-slot round-trip,
  invalid-live-edit, switching continuity, lock-free primitive or resource-budget
  tests. Stopped-audio patch replacement may reset filter history/click.
- **Product and music:** no real guitar, audio render/listening artifact, chart
  fixture, phone browser/usability test, circuit comparison or demand validation.
  Constitution amendments and ADR are proposals; neither approves itself.

## Proposed next engineering ticket

**PAR-002 — Minimal canonical patch codec and validation fixtures**, contingent
on founder approval of ADR-001. Implement only the chosen portable value/codec
contract for the three existing utility types, stable IDs and explicit serial
routing. Require deterministic canonical round trips, precise invalid-input
errors, four independent serialized slot payloads and equal manual/chart-like
fixture outputs. Do not add actual correspondence compilation, runtime exchange,
flash integration or effects. The ADR contains the acceptance details.

The smallest separate safety follow-up is a regression and bounded-output fix
for the overflow example above; it does not depend on approving the new patch
architecture and should precede claims of audio safety. After codec approval and
tests, isolate activation ownership and durable journaling into later tickets.
