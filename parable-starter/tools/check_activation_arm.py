#!/usr/bin/env python3
"""Compile-only PAR-011 / optional PAR-012 Cortex-M7 audit. Never flashes firmware."""
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
p.add_argument('--living', action='store_true', help='also inspect the experimental source lane/wrapper')
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
objects = ('parable_activation', 'parable_activation_audio') + (('parable_living', 'parable_living_audio') if a.living else ())
for name in objects:
    extra = ['-fno-exceptions', '-fno-rtti'] if name.endswith('_audio') else []
    run([compiler, *flags, *extra, '-c', root / 'src' / (name + '.cpp'), '-o', out / (name + '.o')])
source = out / 'layout.cpp'
fields = ['sizeof(parable::Engine)','sizeof(parable::patch::ValidatedPatch)','parable::activation::Runtime::bundleBytes()',
          '4*parable::activation::Runtime::bundleBytes()','sizeof(parable::activation::Runtime)','sizeof(std::atomic<uint32_t>)']
keys = ['engine','typed_document','bundle','pool','runtime','atomic']
if a.living:
    keys += ['source_record','source_profile','source_view','control_snapshot','control_lane','source_scheduler','audio_wrapper','audio_receipt']
    fields += ['sizeof(parable::living::'+x+')' for x in ('Record','Profile','View','Snapshot','Lane','Scheduler','Audio','AudioReceipt')]
source.write_text('#include "'+('parable_living.h' if a.living else 'parable_activation.h')+'"\nextern "C" const uint32_t layout[]={'+','.join(fields)+'};\n')
run([compiler, *flags, '-c', source, '-o', out / 'layout.o'])
run([tool('objcopy'), '--dump-section', '.rodata=' + str(out / 'layout.bin'), out / 'layout.o'])
layout = dict(zip(keys, struct.unpack('<'+str(len(keys))+'I', (out / 'layout.bin').read_bytes())))
audio_names = {}
for name in objects:
    if not name.endswith('_audio'): continue
    audio = out / (name+'.o')
    undefined = run([tool('nm'), '-u', audio])
    names = {line.split()[-1] for line in undefined.splitlines()}
    allowed = {'_ZN7parable3dsp3Rat7processEf', 'memset'} if name=='parable_activation_audio' else {'_ZN7parable10activation7Runtime12processBlockEPKfPfj', 'memset', 'memcpy'}
    assert names <= allowed, undefined
    assembly = run([tool('objdump'), '-dr', audio])
    assert 'dmb' in assembly
    assert not any(x in assembly.lower() for x in ('ldrex','strex','__atomic','__sync'))
    prefix = 'audio' if name=='parable_activation_audio' else 'living-audio'
    (out / (prefix+'.disassembly.txt')).write_text(assembly)
    (out / (prefix+'.undefined.txt')).write_text(undefined)
    audio_names[name] = sorted(names)
sizes = {}
for name in objects:
    text = run([tool('size'), out / (name + '.o')])
    sizes[name] = dict(zip(('text','data','bss'), map(int, text.splitlines()[-1].split()[:3])))
report = dict(compiler=version, flags=flags, layout=layout, objects=sizes,
              audio_undefined=audio_names['parable_activation_audio'], atomic='aligned lock-free uint32 loads/stores; DMB; no exclusive retry/helper',
              firmware_linked=False, physical_tested=False,
              sources={name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in (
                  *('include/parable_activation.h','src/parable_activation.cpp','src/parable_activation_audio.cpp'),
                  *(('include/parable_living.h','src/parable_living.cpp','src/parable_living_audio.cpp') if a.living else ()))}, commands=commands)
if a.living:
    report['living_audio_undefined'] = audio_names['parable_living_audio']
(out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({k:v for k,v in report.items() if k not in ('commands','sources','flags')}, indent=2))
