# SF-013 — exact focused verification

Base main767a3c402b972fc20df8aea0729adf794f4b7a38; no unmerged PR imported.
CMake3.31.6/GNU14.2.0/Python3.12.14. Actual build paths
`/workspace/work/sf013-{release,debug,sanitized}`; configure additionally reused only
unchanged public nlohmann_json3.11.3 sources via
`-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src`.
Fresh repository-root commands:

```sh
cmake -S parable-starter -B /tmp/sf013-release -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/sf013-release --target parable_libra_test parable_libra_lab -j2
ctest --test-dir /tmp/sf013-release -R '^(libra_.*|soundforge_registry|soundforge_registry_negative)$' --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /tmp/sf013-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/sf013-debug --target parable_libra_test parable_libra_lab -j2
ctest --test-dir /tmp/sf013-debug -R '^(libra_.*|soundforge_registry|soundforge_registry_negative)$' --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /tmp/sf013-sanitized -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -O1' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build /tmp/sf013-sanitized --target parable_libra_test parable_libra_lab -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir /tmp/sf013-sanitized -R '^libra_' --output-on-failure --no-tests=error -j2
python3 parable-starter/tools/soundforge/validate_registry.py
python3 parable-starter/tools/libra_reference.py --ngspice /path/to/ngspice44.2 --output /tmp/libra-reference
python3 parable-starter/tools/check_libra_arm.py --compiler /path/to/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --output /tmp/libra-arm
```

Pre-edit utility/registry3/3; final Debug/Release4/4 and affected ASan+UBSan2/2.
`native-observations.txt` records named electrical/feedback/alias/safety observations.
Reference source/hash/version/netlists are in`tests/fixtures/libra/reference.json`.
Actual ngspice:`/workspace/work/par007-ngspice/usr/bin/ngspice`; actual ARM compiler:
`/workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++`,
archive/version pinned in existing`firmware/dependencies.json`.

SPICE independently solves the resistor network with separate base-junction
constant-drop conductances and beta-controlled collector currents; it does not use
native affine coefficients.0.01Ω conducting base-junction resistance regularizes
the equivalent, versus native ideal0.64V threshold. The gain-stage pole is followed
by its declared ideal stiff buffer. Reference1k RC is the pole implementation,
not an extra unmodeled output impedance. Static feedback/load and short low-amplitude
transient comparisons validate the **reduction**, not real device precision.
Transient uses native1x/96k against continuous SPICE; the measured1.481mV error
includes backward-Euler/discrete timing effects. No exhaustive full-transistor work.

Feedback-off/on DC-sine comparison matches nominal healthy0.2V output by explicit
input peaks0.005996V/0.055996V at the gain input. Each circuit's coupling removes
its own DC point; this is not hidden listening gain. The guitar pair uses identical
input/calibration/Level; matching is separate. [Listen/reproduce](../../../samples/libra/README.md).
The existing owned FFT is reused from`rat_measure.h`; unused helper declarations
require RAT_FIXTURES at compile time, but no old RAT test runs. No shared DSP utility
changed; no complete TS808/Klon/Muff/wavefolder suites ran locally, per issue57.

`arm.json` records actual object/layout/calls/static frames/commands; runtime libm
is absent. The sole external audio helper is32-byte error-result memset, verified
from exact compiled r2/r1 arguments. Target timing/helper stack depth and hardware
execution remain unmeasured. `verification.json` separates actual reuse, rework,
first audio and unknowns; line counts are not effort estimates. Remote CI results
are reported separately in PR; existing global checks were not disabled or narrowed.
