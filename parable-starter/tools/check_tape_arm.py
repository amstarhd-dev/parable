#!/usr/bin/env python3
"""Compile/layout/disassemble standalone SF-006. No deployment or target timing."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args();compiler=a.compiler.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
pins=json.loads((root/'firmware/dependencies.json').read_text())
commands=[]
def run(args):
    args=list(map(str,args));commands.append(args);return subprocess.check_output(args,text=True)
assert run([compiler,'-dumpfullversion']).strip()==pins['toolchain']['version']
tool=lambda name:compiler.parent/('arm-none-eabi-'+name)
flags=['-std=c++17','-O2','-Wall','-Wextra','-Wpedantic','-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard',
       '-fno-exceptions','-fno-rtti','-fno-fast-math','-fno-finite-math-only','-fstack-usage','-I'+str(root/'include')]
obj=out/'parable_tape.o';run([compiler,*flags,'-c',root/'src/parable_tape.cpp','-o',obj])
undefined=run([tool('nm'),'-u',obj]);names=sorted(line.split()[-1] for line in undefined.splitlines())
assert set(names)<={'exp','lround','memset'},undefined
assembly=run([tool('objdump'),'-drC',obj]);(out/'tape.disassembly.txt').write_text(assembly)
(out/'tape.undefined.txt').write_text(undefined)
process=assembly.split('<parable::dsp::Tape::process(float)>:',1)[1]
process=re.split(r'\n[0-9a-f]+ <',process,maxsplit=1)[0]
assert not re.search(r'\bblx?\b',process),process
assert not any(name in assembly for name in ('__atomic','__sync','operator new','malloc','mutex','fopen','throw'))
layout=out/'layout.cpp'
layout.write_text('#include "parable_tape.h"\n#include "parable_activation.h"\nextern "C" const uint32_t layout[]={sizeof(parable::dsp::Tape),sizeof(float),parable::dsp::Tape::kMaximumCells,sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};\n')
run([compiler,*flags,'-c',layout,'-o',out/'layout.o'])
run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o'])
values=struct.unpack('<6I',(out/'layout.bin').read_bytes())
assert values[0]<=192 and values[1:]==(4,192000,11040,11288,45224),values
size=run([tool('size'),obj]);text,data,bss=map(int,size.splitlines()[-1].split()[:3]);assert text<=4096 and data==bss==0
report={'compiler':pins['toolchain'],'flags':flags,'object':dict(text=text,data=data,bss=bss),
        'layout':dict(zip(['processor','float','maximum_cells','unchanged_engine','unchanged_bundle','unchanged_runtime'],values)),
        'medium_bytes':{'44100_max':352800,'48000_max':384000,'96000_max':768000},
        'external_calls_whole_object':names,'process_external_calls':[],
        'processing_work':'fixed 3 cell reads/2 writes, two one-poles, one rational SoftClip, head wrap; no iterations',
        'stack_usage':(out/'parable_tape.su').read_text().splitlines(),
        'admission_ceiling_cycles_per_sample':1000,'actual_arm_cycles':'not measured',
        'firmware_linked':False,'sdram_access_observed':False,'physical_tested':False,
        'source_sha256':{s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ['include/parable_tape.h','src/parable_tape.cpp']},
        'commands':commands}
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k not in ('flags','commands')},indent=2))
