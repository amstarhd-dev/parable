#!/bin/bash
set -euo pipefail
export PATH=/workspace/work/baseline-tools/bin:$PATH
repo=/workspace/work/parable-sf017/parable-starter
cache=/workspace/work/sf015-deps/nlohmann_json-src
date -u +%FT%TZ > /workspace/work/sf017-full-start.txt
cmake -S "$repo" -B /workspace/work/sf017-debug -DCMAKE_BUILD_TYPE=Debug -DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON="$cache" > /workspace/work/sf017-debug-configure.log 2>&1
cmake --build /workspace/work/sf017-debug -j2 > /workspace/work/sf017-debug-build.log 2>&1
cmake --build /workspace/work/sf017-release -j2 > /workspace/work/sf017-release-build.log 2>&1
ctest --test-dir /workspace/work/sf017-debug --output-on-failure --no-tests=error -j2 > /workspace/work/sf017-debug-full.log 2>&1 &
debug_pid=$!
ctest --test-dir /workspace/work/sf017-release --output-on-failure --no-tests=error -j2 > /workspace/work/sf017-release-full.log 2>&1 &
release_pid=$!
set +e
wait "$debug_pid"; debug_status=$?
wait "$release_pid"; release_status=$?
printf 'Debug=%s Release=%s\n' "$debug_status" "$release_status" > /workspace/work/sf017-full-status.txt
date -u +%FT%TZ > /workspace/work/sf017-full-end.txt
exit $((debug_status || release_status))
