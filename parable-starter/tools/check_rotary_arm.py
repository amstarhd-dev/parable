#!/usr/bin/env python3
"""Pinned actual standalone M7 rotary compile/layout/call audit, no execution."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args();compiler=a.compiler.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
pins=json.loads((root/'firmware/dependencies.json').read_text());commands=[]
def run(args):
    args=list(map(str,args));commands.append(args);return subprocess.check_output(args,text=True)
assert run([compiler,'-dumpfullversion']).strip()==pins['toolchain']['version']
tool=lambda name:compiler.parent/('arm-none-eabi-'+name)
flags=['-std=c++17','-O2','-Wall','-Wextra','-Wpedantic','-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard',
       '-fno-exceptions','-fno-rtti','-fno-fast-math','-fno-finite-math-only','-fstack-usage','-I'+str(root/'include')]
obj=out/'parable_rotary.o';run([compiler,*flags,'-c',root/'src/parable_rotary.cpp','-o',obj])
undefined=run([tool('nm'),'-u',obj]);names=sorted(line.split()[-1] for line in undefined.splitlines());assert set(names)<={'sin','exp','memset'},undefined
assembly=run([tool('objdump'),'-drC',obj]);(out/'rotary.disassembly.txt').write_text(assembly);(out/'rotary.undefined.txt').write_text(undefined)
audio={}
for function in ['cosine(double) const','advance(parable::dsp::RotorMotion&, bool, double)','process(float, float)']:
    body=assembly.split('<parable::dsp::Rotary::'+function+'>:',1)[1];body=re.split(r'\n[0-9a-f]+ <',body,maxsplit=1)[0]
    calls=re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body)
    assert all('parable::dsp::Rotary::' in c for c in calls),calls
    assert not any(s in body for s in ('malloc','operator new','mutex','fopen','__atomic','__sync','throw'))
    audio[function]=calls
source=out/'layout.cpp';source.write_text('#include "parable_rotary.h"\n#include "parable_activation.h"\nextern "C" const uint32_t layout[]={sizeof(parable::dsp::Rotary),sizeof(parable::dsp::RotorMotion),parable::dsp::Rotary::kDelayCells,parable::dsp::Rotary::kSineSegments+1,sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};\n')
run([compiler,*flags,'-c',source,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o'])
layout=struct.unpack('<7I',(out/'layout.bin').read_bytes());assert layout[0]<=5120 and layout[2:]==(384,257,11040,11288,45224),layout
size=run([tool('size'),obj]);text,data,bss=map(int,size.splitlines()[-1].split()[:3]);assert text<=8192 and data==bss==0
report=dict(compiler=pins['toolchain'],flags=flags,object=dict(text=text,data=data,bss=bss),layout=dict(zip(['processor','assembly','delay_cells_per_band','sine_points','unchanged_engine','unchanged_bundle','unchanged_runtime'],layout)),
            whole_object_undefined=names,audio_calls=audio,stack_usage=(out/'parable_rotary.su').read_text().splitlines(),
            static_work='two fixed mechanical balances; <=one release/capture decision each; four cubic table reads, two fractional reads, no iterative solver',
            target_cycles='not measured',admission_ceiling_cycles_per_sample=2000,firmware_linked=False,physical_tested=False,
            source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ['include/parable_rotary.h','src/parable_rotary.cpp','src/rotary_delay.h']},commands=commands)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ('flags','commands')},indent=2))
