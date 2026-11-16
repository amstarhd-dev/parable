#!/usr/bin/env python3
"""Pinned M7 object, actual layout and strict bounded audio-call audit; no execution.

Adapted from the merged main check_scorpio_arm.py audit conventions.
"""
import argparse
import hashlib
import json
import re
import struct
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--compiler', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
compiler, out = args.compiler.resolve(), args.output.resolve()
out.mkdir(parents=True, exist_ok=True)
pins = json.loads((root/'firmware/dependencies.json').read_text())
commands = []


def run(command):
    command = list(map(str, command)); commands.append(command)
    return subprocess.check_output(command, text=True)


assert run([compiler, '-dumpfullversion']).strip() == pins['toolchain']['version']
tool = lambda name: compiler.parent/('arm-none-eabi-'+name)
flags = ['-std=c++17', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-mcpu=cortex-m7',
         '-mthumb', '-mfpu=fpv5-d16', '-mfloat-abi=hard', '-fno-exceptions', '-fno-rtti',
         '-fno-fast-math', '-fno-finite-math-only', '-fstack-usage', '-I'+str(root/'include')]
obj = out/'parable_coupled_rooms.o'
run([compiler, *flags, '-c', root/'src/parable_coupled_rooms.cpp', '-o', obj])
undefined = run([tool('nm'), '-u', obj])
names = sorted(line.split()[-1] for line in undefined.splitlines())
assert set(names) <= {'powf', 'expf', 'memset', 'memcpy'}, names
assembly = run([tool('objdump'), '-drC', obj])
(out/'coupled_rooms.disassembly.txt').write_text(assembly)
(out/'coupled_rooms.undefined.txt').write_text(undefined)
headers = list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n', assembly, re.M))
audio = {}
for i, header in enumerate(headers):
    name = header.group(1)
    if not any(name.startswith('parable::soundforge::CoupledRooms::'+method+'(') for method in ('process', 'setControls')):
        continue
    body = assembly[header.end():headers[i+1].start() if i+1 < len(headers) else len(assembly)]
    calls = re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)', body)
    # Methods are deliberately small enough to inline all helpers at pinned O2.
    assert not calls, (name, calls)
    assert not re.search(r'\bblx?\s+(?:r\d+|ip)\b|\bbx\s+(?:r\d+|ip)\b', body), name
    assert not any(word in body for word in ('powf', 'expf', '.f64', '__aeabi_d', 'malloc', 'mutex', '__atomic', '__sync', 'throw'))
    audio[name] = dict(calls=calls, double_operations=False)
for method in ('process', 'setControls'):
    assert any(name.startswith('parable::soundforge::CoupledRooms::'+method+'(') for name in audio), method
layout_source = out/'layout.cpp'
layout_source.write_text('#include "parable_coupled_rooms.h"\n#include "parable_activation.h"\n'
                         'extern "C" const uint32_t layout[]={sizeof(parable::soundforge::CoupledRooms),'
                         'parable::soundforge::CoupledRooms::kMaxSamples*4,sizeof(parable::Engine),'
                         'parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};\n')
run([compiler, *flags, '-c', layout_source, '-o', out/'layout.o'])
run([tool('objcopy'), '--dump-section', '.rodata='+str(out/'layout.bin'), out/'layout.o'])
layout = struct.unpack('<5I', (out/'layout.bin').read_bytes())
assert layout[0] <= 256 and layout[1] == 88000 and layout[2:] == (11040, 11288, 45224), layout
size = run([tool('size'), obj])
text, data, bss = map(int, size.splitlines()[-1].split()[:3])
assert text < 8192 and data == bss == 0
report = dict(compiler=pins['toolchain'], flags=flags, object=dict(text=text, data=data, bss=bss),
              layout=dict(zip(('processor', 'maximum_external_buffer', 'unchanged_engine', 'unchanged_bundle', 'unchanged_runtime'), layout)),
              whole_object_undefined=names, audio_calls=audio,
              stack_usage=(out/'parable_coupled_rooms.su').read_text().splitlines(),
              work_bound='8 delay reads/writes and one-pole absorption states; 2 four-port Householder mixes; one two-port coupling; 3 scalar control ramps; fixed loop bounds, no iteration/search/libm/calls in process',
              target_cycles='not measured', firmware_linked=False, physical_tested=False,
              source_sha256={s: hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ('include/parable_coupled_rooms.h', 'src/parable_coupled_rooms.cpp')},
              commands=commands)
(out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps({k: v for k, v in report.items() if k not in ('commands', 'flags')}, indent=2))
