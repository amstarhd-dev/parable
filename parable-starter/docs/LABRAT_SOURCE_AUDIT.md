# LabRAT source and reference audit (PAR-007)

The source audit preceded DSP implementation. Full exact file SHA-256 records,
revisions and offline-tool pin are in
[provenance.json](../third_party/labrat/provenance.json). No floating upstream
branch is a build dependency. The selected port needs only C++17/standard math;
DaisySP, JUCE, SharpSoundDevice and AudioLib are not imported or linked.

| Candidate, exact revision | Circuit content / omissions | Reuse decision and license |
| --- | --- | --- |
| [PiRAT](https://github.com/nezetic/pirat/tree/adeb07e8799c70ef74f1051a09c3b09e53692dbe) | `TFGain.cpp` contains component-derived third-order feedback polynomial; `PiRATDist.cpp` adds estimated input roll-off, ±4 V op-amp clipping, fixed silicon spline and separate tone/output filters. No finite GBW/slew, coupled diode/filter response, individual diode failure or full JFET model. Its upstream CMake fetches unpinned DaisySP/libDaisy. | BSD-2-Clause, copyright Cedric TESSIER; verify `LICENSE.txt`, retain it. Selected feedback equations/pot law, not wholesale DSP. Existing Daisy targets are not Seed3 performance evidence. |
| [Rodent.V2](https://github.com/ValdemarOrn/SharpSoundPlugins/tree/a791098f68f95c6f3548c9d9853f54ed33043d90/Rodent.V2) | `TFGain.cs` is the same analytic feedback model; `Rodent.cs` gives the staging; `Analysis/transfer.m`, `Transfer.nb`, `Rat.asc`, `components.jpg` expose derivation/circuit context. Windows/.NET VST needs external AudioLib. `Rat.asc` references absent `opamp.sub`; supplied image, netlist and DSP do not all use the same pot value (150 kΩ image/code versus 15 kΩ in the saved ASC). It is not a verified vintage reference dataset. | Root `license.txt` explicitly MIT, Valdemar Erlingsson. Retain MIT alongside BSD for the derivative feedback polynomial. No UI/graphics/VST host copied. R.G. Keen/Jack Orman schematic image is viewed/cited, not redistributed under an assumed code license. |
| [BYOD Mouse Drive](https://github.com/Chowdhury-DSP/BYOD/tree/1cf22b6ac802b9dc33cfc9f8dd6af5b3c3e40bc9/src/processors/drive/mouse_drive) | Editable WDF input/feedback/clip components, useful R-type approach. `MouseDriveWDF.h` uses fixed open-loop gain 100 and idealized diode pair; processor describes itself as the clipping stage, with no classic RAT Filter. Full host uses JUCE/chowdsp. Not a complete measured LM308 device model. | GPL-3.0 root LICENSE. Audited as an alternative and component cross-check, no code or schematic graphics imported. Adopting this source would require a deliberate project distribution/license decision. |
| [AudioLib](https://github.com/ValdemarOrn/AudioLib/tree/9d4dfcab2136427eb7b87fdb47098f439f53a16b) | Rodent's `Bilinear` and `SplineInterpolator` dependency lives outside the plugins repository. No source license found in the audited tree (NUnit package licenses do not license AudioLib). | Not imported. Standard bilinear substitution is independently implemented with fixed binomial arrays; no AudioLib source expression is copied. This prevents a hidden dependency-license assumption. |

### Cost estimates and portability limits

PiRAT's six `Transfer` stages retain four 50-float histories each even in mono:
roughly **4,800 history bytes**, plus coefficients/module/splines/faders, about
**5 KiB-class state** before platform buffers. The filter loops evaluate about
**44 coefficient multiplies per sample** (both internal channels), plus cubic
interpolation, gains/mix/clamps and pot powers depending on invocation granularity.
These are source-shape estimates, not compiled Seed3 CPU/memory measurements.
Rodent's mono filters imply about 22 coefficient multiplies plus spline math, but
it additionally allocates variable double buffers (at least 16N bytes for signal/
clean per N-frame block) and needs CLR/host/library state. Its total is not an
embedded RAM estimate. BYOD's four-port WDF plus reactive/diode evaluations would
need a pinned framework port/benchmark; no honest whole-program Seed3 figure is
available from this audit. Only the selected standalone native port has an actual
ARM object/layout measurement, recorded in the report. None of the upstream
Daisy usage is evidence of tested Seed3 hardware compatibility.

## Why this foundation

The feedback topology is analytically identifiable, already available under
permissive source licenses and directly exposes the important 560 Ω/4.7 µF,
47 Ω/2.2 µF, Gain and feedback-capacitor relationships. Porting that kernel and
extending omitted stages is smaller and more auditable than adopting a VST host,
rebuilding a WDF framework or generating an unrelated new distortion architecture.
The resulting processor remains one RAT-derived model with component variations.

Necessary extensions are explicit in [ADR-003](adr/ADR-003-labrat-circuit-model.md):
finite-open-loop response plus idealized rail/slew limits, actual input RC values,
a coupled nonlinear diode/filter/coupling network, explicit disconnected branches,
and a passive output load/Volume model. Fixed arrays replace general temporary
containers. Double filter history avoids fragile single-precision high-order
coefficient cancellation. Actual sizes/work estimates are in the lab report;
upstream CPU/RAM numbers have **not** been benchmarked or inherited.

## Diodes and lookup provenance

PiRAT's `Splines.cpp` reproduces Rodent's `Splines.D1N914TF`. Its spline at zero
is about −11.17 mV, and the available source does not include the exact simulation
netlist generating that table. It is not suitable as a complete causal failure
model. As a cross-check, a 1 kΩ-fed exponential pair using Is=2.52 nA, n=1.752,
Vt=25.85 mV differs from that spline by at most **22.19 mV** over −4..4 V (1,001
points). This is consistency with an upstream approximation, not calibrated
1N914 device identification. The selected nominal diode parameters remain a
reduced model; series resistance, capacitance/recovery and tolerances are absent.

The native table instead inverts the actual prepared nonlinear port equation;
removing a diode deletes its current branch. Independent ngspice diode/filter
transients verify that reduced network. No PAR-006 sigmoid/drive curve is reused.

## Reference circuit and source-access limits

Reference designation: **classic LM308/silicon RAT, 100 kΩ distortion-pot variant**,
a nominal reconstruction of the late-1970s design lineage. The 560/47 Ω branches,
4.7/2.2 µF capacitors, 100 pF feedback capacitor, 1 kΩ clipping resistor and
100 kΩ/1.5 kΩ/3.3 nF Filter are identifiable across these published references.
BYOD's circuit depiction uses 100 kΩ; Rodent/PiRAT select 150 kΩ. Our choice is
recorded explicitly rather than silently calling both the same factory revision.
The source image also identifies 30 pF external LM308 compensation; this is not
the 100 pF feedback capacitor varied in the third experiment.

ElectroSmash's ProCo RAT analysis was located at
<https://www.electrosmash.com/proco-rat> (issue #12 also supplies
<https://www.electrosmash.com/proco-rat-analysis/pedals/distortion/pro-co-rat-distortion.html>), but access returned a proxy CONNECT
HTTP 403. It was **not read**, and no secondary summary is passed off as a review
of it. Other attempted original schematic/datasheet endpoints were unavailable;
no manufacturer PDF was successfully verified. A factory year/serial/BOM and 9 V
LM308 dynamic-parameter authentication remain unfulfilled evidence. Do not label
this a verified 1978/1979 factory-model emulation. The reduced nominal circuit,
assumptions and selected source disagreements are available for review now.

## Independent simulation and licenses

Offline ngspice **44.2**, Debian package **44.2+ds-1**, was downloaded/extracted in
user-owned workspace, not installed into the system or linked into Parable.
Archive URL/SHA are pinned in provenance.json. Package copyright identifies BSD-
3-Clause for the main simulator with file-specific documentation/model exceptions;
no ngspice source/library or vendor LM308 macromodel is redistributed here.

`tools/rat_spice_reference.py` generates our own explicit resistor/capacitor/diode/
controlled-source netlists, runs independent AC/transient analyses and records
version/netlist/fixture hashes. The committed CSV fixtures are its actual output,
not numbers generated by the C++ implementation. They describe a **reduced**
op-amp and ideal JFET buffer, not independent validation of a full LM308/JFET.
No vendor model is silently substituted when the upstream `opamp.sub` is missing.

Borrowed-source notices accompany both source and compiled/audition artifacts.
The repository still has no overall project LICENSE; upstream notices do not
license Parable's remaining code. That ownership decision remains open.
