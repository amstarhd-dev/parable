# SF-018 — minimal reproduction and evidence

Main af159d833bdd0b0e17ccc59fcf7edfa506ed98f5. No unmerged implementation imported.
CMake3.31.6/GNU14.2.0/Python3.12.14 here. Source fetch is existing pinned
nlohmann_json3.11.3; actual commands additionally used only its unchanged public
cache:`-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src`.
Actual build directories:`/workspace/work/sf018-{release,debug,sanitized}`.
Fresh repository-root reproduction:

```sh
cmake -S parable-starter -B /tmp/sf018-release -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/sf018-release -j2
ctest --test-dir /tmp/sf018-release --output-on-failure --no-tests=error -R '^(capricorn_.*|soundforge_registry|soundforge_registry_negative)$' -j2
cmake -S parable-starter -B /tmp/sf018-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/sf018-debug --target parable_capricorn_test parable_capricorn_lab -j2
ctest --test-dir /tmp/sf018-debug --output-on-failure --no-tests=error -R '^(capricorn_.*|soundforge_registry|soundforge_registry_negative)$' -j2
cmake -S parable-starter -B /tmp/sf018-sanitized -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -O1' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build /tmp/sf018-sanitized --target parable_capricorn_test parable_capricorn_lab -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir /tmp/sf018-sanitized --output-on-failure --no-tests=error -R '^capricorn_' -j2
python3 parable-starter/tools/soundforge/validate_registry.py
python3 parable-starter/tools/capricorn_reference.py --ngspice /path/to/ngspice44.2 --output /tmp/capricorn-reference
python3 parable-starter/tools/check_capricorn_arm.py --compiler /path/to/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --output /tmp/capricorn-arm
```

Reference generation reuses unchanged`rat_spice_reference.run_spice`, no NumPy.
Pinned actual ngspice executable:`/workspace/work/par007-ngspice/usr/bin/ngspice`;
version/digest recorded in`tests/fixtures/capricorn/reference.json`, along with exact
generator/netlist/fixture hashes. Netlists use relative output paths and regenerate
without workspace-specific hashes. ARM compiler is pinned by existing
`firmware/dependencies.json`; actual executable was under
`/workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/`.

`pre-edit.txt`: unchanged utility/registry3/3 before DSP edits. Final new/registry
checks4/4 each in Debug/Release; final affected ASan+UBSan2/2. `native-observations.txt`
records sparse electrical errors and the1x/4x/8x alias comparison. This is not a
swept alias specification or full nonlinear physical-circuit validation. The diode
companion fixture deliberately uses4x/192k; audio and remaining checks use8x.
Q2 AC includes analytical input-buffer attenuation; Tone compares measured jack/
diode probe ratios and includes small output-coupling deviations. Only the owned
FFT from`rat_measure.h` runs; its unused helper declarations require the existing
RAT_FIXTURES macro, and no old RAT circuit test executes. No shared utilities changed.

`arm.json`: actual object/layout/reachable calls/static frames/commands; compile
inspection only, not firmware linking or hardware execution. The four quiet host
measurements are actual full-processor repeat loops in the native lab. Nine final
WAVs are byte-identical between two separate native runs; the second follows all
builds/tests for less-contended timing. [Listening and source recipe](../../../samples/capricorn/README.md).
`verification.json` separates results, reuse, rework and unknowns. No complete old
TS808/Klon/Muff/wavefolder suites ran locally; issue68 explicitly limits per-effect
regression scope. Global branch CI is left intact and reported separately in PR.
