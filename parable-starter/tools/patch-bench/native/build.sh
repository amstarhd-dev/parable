#!/bin/bash
set -eu
em++ -std=c++17 -O3 -DPARABLE_RAT_BENCH_FIR97 -fno-exceptions -fno-rtti \
  bench.cpp parable_rat.cpp parable_scorpio.cpp hall_core.cpp parable_coupled_rooms.cpp \
  -s STANDALONE_WASM=1 -s ALLOW_MEMORY_GROWTH=0 --no-entry \
  -s EXPORTED_FUNCTIONS=_bench_prepare,_bench_controls,_bench_sample -o bench.wasm
node -e 'const fs=require("fs");const mod=new WebAssembly.Module(fs.readFileSync("bench.wasm"));console.log(WebAssembly.Module.imports(mod));const api=new WebAssembly.Instance(mod,{}).exports;api._initialize();for(let id=1;id<=5;id++){if(!api.bench_prepare(0,id))throw Error(id);for(let n=0;n<96000;n++){const x=api.bench_sample(0,0.1*Math.sin(n*0.05));if(!Number.isFinite(x))throw Error(id)}}'
