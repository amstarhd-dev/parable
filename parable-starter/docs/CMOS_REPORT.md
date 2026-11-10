# SF-002 — experimental CD4049 inverter distortion

This is a standalone, source-derived **reduced two-inverter signal path**, not a
calibrated commercial clone or approved Gemini voice. No Engine, patch format,
correspondence, astrology, hardware adapter or LabRAT DSP changes. The research
choice is [ADR-004](adr/ADR-004-cmos-inverter-model.md); musical approval remains
pending. [Provenance](../third_party/cmos/provenance.json) and
[attribution](../third_party/cmos/ATTRIBUTION.md) identify every evaluated source.

## Starting point and source decision

Fresh main `2cc9412c1968d1c229bcbedba7fb6e19dfd6f416`, after the founder merged
PR #15 including the final license correction. All required repository documents
were present and read. The independent review exists as COMMENTED, followed by
the founder's merge; a formal GitHub APPROVED review is **not recorded**. This
does not manufacture evidence for the issue's literal approval precondition.
Clean baseline: Debug **10/10**, Release **10/10**, full pinned ARM build passed.

Borrowed the MIT ACME.jl `mosfet` current/Jacobian at its published DAFx commit,
using Köper/Holters' extended measured CD4049 fit and nominal signal components.
The original work supplies nonlinear device physics and measured-model research;
the local engineering gap is a dedicated bounded two-output solver, discretized
passive circuit and native validation. No Julia/framework/dependency import.
The exact MIT notice is retained. The scientific article is CC-BY-3.0 and cited;
no schematic graphics or audio imported. Obsidian AGPL code/fit/curves are audit
only. TI's actual PSpice archive is a binary logic/delay model, rejected as analog
transfer truth. Tonepad's full drawing could not be retrieved; no claim to have
inspected or imported it. No new legal allowlist exception. Global Parable
licensing remains unspecified.

## Circuit, controls and limitations

All capacitors use Farads, resistors Ohms and supply Volts. Nominal configuration:

| Branch | Value |
| --- | --- |
| IC supply | Ideal stiff 9 V |
| Input | 100 kOhm series, 68 nF AC coupling |
| First feedback | (100 kOhm + Drive × 1 MOhm) parallel 51 pF |
| Interstage coupling | 33 nF |
| Second feedback | 1 MOhm parallel 100 pF |
| Output | 10 uF; 10 kOhm Volume divider; 1 MOhm external load |

Complementary MOSFET drain currents include fitted transconductance/threshold
dependence and PMOS channel-length modulation. Feedback self-bias is solved from
the same current relationships; nominal 9 V DC bias is **3.482808651 V**. Input
and interstage coupling block DC, feedback affects gain/bandwidth and output
loading enters the nonlinear solve. Drive/Volume are linear electrical fractions
0..1, not measured mechanical taper. Input 1.0 means one model Volt; normalized
output is jack Volts / 4. No physical codec/guitar-level calibration is implied.

The original battery path (1 kOhm/330 uF dropper, protection) is **excluded**.
Two gates at a stiff 9 V are not the complete original power circuit. Unused gates,
intrinsic gate/junction capacitance, body/protection currents, temperature,
component tolerance/noise and package/power limits are unsupported. This is a
CD4049UB/UBE-derived model, not an assertion of CD4069 equivalence.

The published fit is characterized at |Vgs| 2.5..6 V. Fitted parameter functions
freeze at endpoints outside that window while actual gate voltage stays in the
current equation. This unvalidated extrapolation avoids nonpassive polynomial
behavior; it is not manufacturer characterization. Configure accepts 6..9 V
as a numerical envelope; source-reference evidence here is **9 V** only.

Each internal sample uses trapezoidal capacitor companions and a 2×2 analytic
Newton solve, maximum 16 loop evaluations/updates, bounded output-voltage steps.
Final actual residual is rechecked, accepted at <=1 nA. Failure records a counter,
resets circuit histories to DC bias and returns zero for that internal phase.
Finite inputs clamp to +/-2 model V; output clamps +/-1; tiny histories are tidied.
These are numerical containment, not analog fault sounds. Nonfinite input returns
zero without advancing history. Configure/reset require stopped audio.

Whole-circuit 4x oversampling uses a prepared 193-tap FIR on interpolation and
decimation (48 base-sample FIR delay), with a 1x reference mode. No allocation,
throw, lock, I/O, coefficient preparation or unbounded loop occurs in processing.
Normal references, measurements and listening renders **require zero fallback**.
Random-bit/extreme-float tests establish containment; they do not promise physical
behavior or uninterrupted convergence under arbitrary overload.

## Two component experiments and measured differences

`healthy`, `coupling_small`, `feedback_low` are the only accepted laboratory names.
Unknown names throw on the host; malformed native configuration returns false
before changing the prepared state. Ordinary Drive/Volume are separate.

| Experiment | Physical change | Measured small-signal fundamental, 199 / 996 Hz |
| --- | --- | --- |
| Healthy | Nominal circuit | 0.00123947 / 0.00141975 |
| coupling_small | Interstage 33 nF -> 330 pF, 100× wrong part | 0.0000239514 / 0.0000981888 |
| feedback_low | Second feedback 1 MOhm -> 100 kOhm, leak/parallel path equivalent | 0.000237992 / 0.000879409 |

Values are normalized output amplitudes at 0.1 mV sine input, Drive/Volume .5,
4x/48 kHz, coherent 4096-sample window after 32768 settling samples. They show
changed AC coupling/feedback, not separate arbitrary curves. Full frequency,
level, THD+N/DC/second-harmonic measurements accompany the WAVs. No bias-open,
supply-fault, added effect or third fault implemented.

## Independent checks and host evidence

Offline **ngspice 44.2** independently solves literal published MOSFET equations
and passive circuit stamps. It does not consume native-generated expected data.
The existing pinned package/process wrapper is reused. Reference generation saves
netlists/logs/startup and a manifest of netlist/fixture hashes. DC includes no load
and 100 kOhm load; transient includes three healthy Drive settings and both faults,
0.1 V / 997 Hz, Volume .5, 9 V. Simulator maximum step .5 us, after .2 s settling;
native is 96 kHz / 1x. Compare all five nodes, not only the audible output.

| Check | Actual error | Acceptance gate |
| --- | ---: | ---: |
| 362 DC points | 1.07857918e-8 V peak | <1e-5 V |
| 960 transient rows × 5 nodes | 0.000814933 V RMS | <0.005 V |
| Same transient nodes | 0.0231587 V peak | <0.05 V |

This is independent **solver agreement for the selected reduced model**, not new
physical-unit calibration. TI supports identity/operating bounds; its typical
5/10 V transfer plots are not digitized or claimed as a quantitative 9 V match.
The original paper's physical measurements do not become measurements of our code.
Early SPICE DC convergence found an out-of-domain root; physical-node initial
guesses and explicit domain rejection fixed the reference, rather than weakening
the native comparison. See the measured work log for rework.

Coherent 8800.78125 Hz / 0.1 V, Drive .9 / Volume .5: selected folded-third line
is **-14.2949 dBc at 1x**, **-102.440 dBc at 4x**. This is one alias fixture;
88.145 dB difference is **not total alias rejection**, a perceptual guarantee or
a broad band alias bound. 1x/4x discretization also changes circuit response.

Fresh final Debug **13/13**, Release **13/13**, always-active checks. Native CMOS
target: **1,257,836 checks**, zero observed processing allocations. Coverage:
all component endpoints and just-outside rejection, rate/control bounds, state
preservation, unknown names, sample/block/in-place/reset identity, stopped fault
changes, silence/DC recovery, nonfinite/denormals/random bits/hot floats, zero
Volume, numerical reference and selected alias/DC/spectrum gates. CLI tests
exercise deterministic owned test PCM16 at 44.1 kHz, source preservation including
hard links, malformed/empty arguments and output metadata. Generic utility target:
47 checks. Registry: five assets, seven positive/negative test groups; exact MIT /
BSD-2-Clause import policy unchanged. ASan+UBSan commands below cover four targets.
All **sixteen LabRAT WAV/report files and ten protected source/reference hashes**
pass unchanged in both configurations; the baseline fixture was not refreshed.

## Listening material

Release CI publishes `parable-cmos-auditions` (seven-day retention). Eight
9-second, 48 kHz, mono PCM16 WAVs: dry; healthy low/mid/high Drive; weak/strong
input (.1×/2×); small coupling capacitor; low feedback resistor. Same native C++
implementation as tests/ARM. Shared harmonic plucks/strums at .05 model V peak,
Volume .5; **synthetic, not recorded guitar**. Optional `--input` uses the existing
bounded mono PCM16 reader; provide trailing silence for the truncated output tail.

Raw RMS/peak/DC, exact controls/changed components, applied gains, final levels and
all-phase solver work are in `renders.csv`; full circuit in `configuration.json`.
Each processed file has 1,728,000 internal solves, zero failure, observed maximum
3..6 loop evaluations. Raw peaks do not reach the numerical output rail; runner
rejects that condition or any fallback before allowing normalization.

Shared RMS target is **-24.456374 dBFS**, limited by the .89 peak constraint.
It is whole-file RMS matching, not LUFS/perceptual matching. Coupling fault raw
RMS is -56.26 dBFS and receives **38.925×** matching gain: hear its spectrum with
that large attenuation disclosed, not an invented loud malfunction. No cabinet,
amp, alignment or other sonic post-processing. Dry bypasses the circuit.
Artifact includes source/compiler identity, WAV/source SHA-256, provenance/notices,
registry and SPICE manifest. `SOURCE_COMMIT` may be a synthetic PR merge checkout;
`HEAD_COMMIT` records the submitted head. Neither implies PR merge or approval.

## Actual ARM resource evidence

Existing pins/flags/linkers unchanged: xPack GNU Arm Embedded GCC 14.2.1-1.1,
libDaisy `1289a000bc29dae1bc0ed4129cfa2df40ef71574`, Cortex-M7/fpv5-d16/hard,
O2, exceptions/RTTI disabled, no fast/finite-only math. Build compiles actual
`parable_cmos.cpp`, checks undefined allocation/throw/I/O symbols and measures
layout separately with the ARM compiler. `report.json`, object, symbol list,
ELF attributes and `.su` compiler stack frames are CI artifacts.

| Kernel | Object text / data / BSS | ARM instance | Host instance |
| --- | ---: | ---: | ---: |
| New CMOS | 5604 / 0 / 0 bytes | 2128 bytes | 2136 bytes |
| Existing RAT, unchanged | 6452 / 0 / 0 bytes | 10752 bytes | 10760 bytes |

Object text includes prepared-method code; it is not final linked cost. Unresolved
symbols are `sin`, `cos`, `memcpy`, `memset`; trig is preparation only. Largest
reported own frame: circuitSample 344 bytes; inverter 80, process 56, processBlock
24. These are compiler frames, not runtime high-water or complete library stacks.
Static work: 386 FIR multiply/accumulates/base sample; four circuit solves, at most
64 Newton updates plus final residual evaluations (up to 136 inverter evaluations,
272 channel evaluations). These are bounds/operation counts, **not target cycles**.
No target CPU, real-time deadline, latency, power or physical memory measurement.

CMOS is **not linked or activated** in firmware. Existing three firmware binaries
and RAT object are byte-identical to clean main: minimal internal 74008 bytes;
full diagnostic SRAM/QSPI 196292 each. The intentionally attempted full-codec
internal-flash probe still exceeds its 128 KiB layout by **65380 bytes**, the
existing narrowly recognized outcome. No FLASH enlargement or dependency change.
Existing vendor warnings remain; successful ELF builds prove neither Seed3 boot
nor actual audio hardware/bootloader compatibility.

## Reproduction and file roles

From repository root; host CMake >=3.15, C++17, Python >=3.9. Initial and final
builds use ordinary pinned FetchContent, no source override. Actual local output
paths were `/workspace/work/sf002-{before,final}-{Debug,Release}`:

```bash
cmake -S parable-starter -B /workspace/work/sf002-final-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspace/work/sf002-final-Debug -j2
ctest --test-dir /workspace/work/sf002-final-Debug --output-on-failure --no-tests=error
cmake -S parable-starter -B /workspace/work/sf002-final-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /workspace/work/sf002-final-Release -j2
ctest --test-dir /workspace/work/sf002-final-Release --output-on-failure --no-tests=error
/workspace/work/sf002-final-Release/parable_cmos_lab --output /workspace/work/sf002-final-auditions
python3 parable-starter/tools/soundforge/validate_registry.py
python3 parable-starter/firmware/build.py --cache /workspace/work/par005-fresh-cache --output /workspace/work/sf002-after-arm
```

ARM script downloads/verifies missing pinned dependencies; Linux x86_64/Python
>=3.12. For optional reference regeneration, use the existing LabRAT provenance's
SHA-pinned ngspice package, then compare fixtures/manifest to committed files:

```bash
python3 parable-starter/tools/cmos_spice_reference.py --ngspice /workspace/work/par007-ngspice/usr/bin/ngspice --output /workspace/work/sf002-spice-final --fixtures /workspace/work/sf002-fixtures-final
diff -rq parable-starter/tests/fixtures/cmos /workspace/work/sf002-fixtures-final
cmake -S parable-starter -B /workspace/work/sf002-sanitize -DCMAKE_BUILD_TYPE=Debug '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined -no-pie'
cmake --build /workspace/work/sf002-sanitize --target parable_cmos_test parable_cmos_lab parable_soundforge_test -j2
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir /workspace/work/sf002-sanitize -R '^(cmos_|soundforge_utilities)' --output-on-failure --no-tests=error
```

Files: `include/parable_cmos.h` is the stopped-configuration/fixed-state API;
`src/{parable_cmos.cpp,cmos_device.h}` implement circuit/device; `tools/cmos_*`
are model-specific host renderer/measurements/offline reference; `tests/test_cmos*`
and `fixtures/cmos` supply safety/CLI/independent gates. `third_party/cmos` retains
notices/audit. Shared `excitation.h` replaces only LabRAT's owned local generator.
CMake/host workflow add tests/listening artifact; ARM recipe adds object evidence.
Registry/README/status and this report/work log document actual evidence. No WAV,
SDK, PDF, TI model or upstream framework binary enters Git.

Unproven: physical circuit/unit fit, real guitar, founder musical acceptance,
hardware boot/audio/CPU/latency/power/stack and runtime integration. Open design
questions: fitted-gate extrapolation accuracy, whether supply-dropper dynamics
matter musically, and acceptable resource cost on hardware. **Next smallest test:**
founder listens to healthy vs the two experiments, then compare the chosen healthy
configuration against an owned physical CD4049 circuit at a measured IC rail.
Any patch integration or expanded physics requires its own approved scope.
