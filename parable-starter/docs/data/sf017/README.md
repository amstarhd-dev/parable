# SF-017 reproducibility and raw evidence

Repository-root commands; CMake3.31.6, GNU14.2.0, Python3.12.14 here. Initial
configuration fetches pinned nlohmann_json3.11.3. This run reused only its unchanged
public source cache with`-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src`.
No unmerged implementation or scheduling changes were imported. Main baseline
`af159d833bdd0b0e17ccc59fcf7edfa506ed98f5` was already verified38/38 Debug/Release;
recorded baseline summaries are retained here. Issue66 explicitly waives repeating it.

The actual final directories were`/workspace/work/sf017-debug`, `sf017-release`
and`sf017-sanitized`. The following commands use`/tmp` for a fresh reproduction:

```sh
cmake -S parable-starter -B /tmp/sf017-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/sf017-debug -j2
ctest --test-dir /tmp/sf017-debug --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /tmp/sf017-release -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/sf017-release -j2
ctest --test-dir /tmp/sf017-release --output-on-failure --no-tests=error -j2
cmake -S parable-starter -B /tmp/sf017-sanitized -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -O1' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build /tmp/sf017-sanitized --target parable_scorpio_test parable_scorpio_lab -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir /tmp/sf017-sanitized --output-on-failure --no-tests=error -R '^scorpio_' -j2
python3 parable-starter/tools/soundforge/validate_registry.py
python3 parable-starter/tools/check_scorpio_arm.py --compiler /path/to/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++ --output /tmp/scorpio-arm
python3 parable-starter/tools/scorpio_reference.py --ngspice /path/to/ngspice44.2 --output /tmp/scorpio-reference
```

ARM14.2.1-1.1 archive/hash is pinned in`firmware/dependencies.json`; same offline
ngspice44.2 executable as the existing main reference tools (SHA recorded in
`tests/fixtures/scorpio/reference.json`). Generator has no NumPy dependency.
Compare regenerated`.csv` numerically/bytewise on the pinned simulator; generated
netlists contain the chosen absolute output path, so their checksums vary with
output directory. Stored hashes authenticate the actual original netlists/fixtures.
Independent full transistor SPICE solve, not C++-generated expected waveforms.
One healthy loaded transient uses independent SPICE`.tf` source resistance;
static output impedance under drift is not independently gated. High-note check
compares1x/no-ADAA to4x/ADAA and includes filter/ADAA differences; not a sweep or
comparison against a measured pedal. Sampling probes are pre-decimation internals.

[Listening recipe](../../../samples/scorpio/README.md). Native CLI rejects existing
outputs, verifies reset/rerender bit-exact, and records actual host full-processor
cost. Captured source hashes/producing commit travel with the audio. Local disk
pressure required reversible, SHA-verified gzip archiving of completed Debug build
objects after linking; test executables, inputs, sources and logs were retained.
This workspace housekeeping does not affect fresh-checkout reproduction.

`arm-report.json`: actual commands, symbols, disassembly call audit, layout/static
frames and work counts; a compile/object audit, not firmware link or target run.
`targeted.txt`, `sanitizers.txt`: actual focused results.
`verification.json`: final results and evidence status; full logs added on completion.
Remote CI state is reported separately in the PR, never inferred from local success.

`work-metrics.json` records observed elapsed time, unchanged reused line counts,
new model-specific code, copied/adapted helpers and known rework. Line counts are
not labor percentages; active labor was not tracked. `baseline-results.json`
retains the38 individual pre-edit main exits per configuration, without importing
any open-PR implementation or verification profile. The full final suite found
one stale registry **asset-count assertion** (12 rather than13); only that count
was corrected. Original full logs plus focused two-test registry reruns in both
configurations are retained. No negative-policy, physics or audio threshold changed.
