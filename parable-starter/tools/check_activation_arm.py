#!/usr/bin/env python3
"""Compile-only PAR-011 Cortex-M7 audit. Never links or flashes firmware."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--compiler', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
compiler = a.compiler.resolve()
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=True)
pins = json.loads((root / 'firmware/dependencies.json').read_text())
version = subprocess.check_output([compiler, '-dumpfullversion'], text=True).strip()
assert version == pins['toolchain']['version'], (version, pins['toolchain']['version'])
tool = lambda name: str(compiler.parent / ('arm-none-eabi-' + name))
flags = ['-std=c++17', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-mcpu=cortex-m7', '-mthumb', '-mfpu=fpv5-d16',
         '-mfloat-abi=hard', '-fstack-usage', '-I' + str(root / 'include')]
commands = []
def run(args):
    args = list(map(str, args)); commands.append(args)
    return subprocess.check_output(args, text=True)
for name in ('parable_activation', 'parable_activation_audio'):
    extra = ['-fno-exceptions', '-fno-rtti'] if name.endswith('_audio') else []
    run([compiler, *flags, *extra, '-c', root / 'src' / (name + '.cpp'), '-o', out / (name + '.o')])
source = out / 'layout.cpp'
source.write_text('#include "parable_activation.h"\nextern "C" const uint32_t layout[]={sizeof(parable::Engine),sizeof(parable::patch::ValidatedPatch),parable::activation::Runtime::bundleBytes(),4*parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime),sizeof(std::atomic<uint32_t>)};\n')
run([compiler, *flags, '-c', source, '-o', out / 'layout.o'])
run([tool('objcopy'), '--dump-section', '.rodata=' + str(out / 'layout.bin'), out / 'layout.o'])
layout = dict(zip(('engine','typed_document','bundle','pool','runtime','atomic'), struct.unpack('<6I', (out / 'layout.bin').read_bytes())))
assert layout == dict(engine=11040, typed_document=236, bundle=11288, pool=45152, runtime=45224, atomic=4), 'SF-005 must preserve the pinned M7 prepared-pool budget'
audio = out / 'parable_activation_audio.o'
undefined = run([tool('nm'), '-u', audio])
names = {line.split()[-1] for line in undefined.splitlines()}
assert names <= {'_ZN7parable3dsp3Rat7processEf', '_ZN7parable3dsp4Cmos7processEf', 'memset'}, undefined
assembly = run([tool('objdump'), '-dr', audio])
assert 'dmb' in assembly
assert not any(x in assembly.lower() for x in ('ldrex','strex','__atomic','__sync'))
(out / 'audio.disassembly.txt').write_text(assembly)
(out / 'audio.undefined.txt').write_text(undefined)
sizes = {}
for name in ('parable_activation', 'parable_activation_audio'):
    text = run([tool('size'), out / (name + '.o')])
    sizes[name] = dict(zip(('text','data','bss'), map(int, text.splitlines()[-1].split()[:3])))
report = dict(compiler=version, flags=flags, layout=layout, objects=sizes,
              audio_undefined=sorted(names), atomic='aligned lock-free uint32 loads/stores; DMB; no exclusive retry/helper',
              firmware_linked=False, physical_tested=False,
              sources={name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in (
                  'include/parable_activation.h','src/parable_activation.cpp','src/parable_activation_audio.cpp')}, commands=commands)
(out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({k:v for k,v in report.items() if k not in ('commands','sources','flags')}, indent=2))
