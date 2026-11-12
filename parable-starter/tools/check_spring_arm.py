#!/usr/bin/env python3
"""Actual pinned M7 spring object/state/call audit; no target execution/link."""
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
flags=['-std=c++17','-O2','-Wall','-Wextra','-Wpedantic','-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-fno-exceptions','-fno-rtti','-fno-fast-math','-fno-finite-math-only','-fstack-usage','-I'+str(root/'include')]
obj=out/'parable_spring.o';run([compiler,*flags,'-c',root/'src/parable_spring.cpp','-o',obj])
undefined=run([tool('nm'),'-u',obj]);names=sorted(line.split()[-1] for line in undefined.splitlines());assert set(names)<={'cos','sin','exp','sqrt','nextafter','memset'},undefined
assembly=run([tool('objdump'),'-drC',obj]);(out/'spring.disassembly.txt').write_text(assembly);(out/'spring.undefined.txt').write_text(undefined)
audio={}
for function in ['substep(double)','process(float)']:
    body=assembly.split('<parable::dsp::Spring::'+function+'>:',1)[1];body=re.split(r'\n[0-9a-f]+ <',body,maxsplit=1)[0]
    calls=re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body)
    assert all('parable::dsp::Spring::' in c or c in {'sqrt','nextafter','memset'} for c in calls),calls
    assert not any(s in body for s in ('malloc','operator new','mutex','fopen','__atomic','__sync','throw'))
    audio[function]=calls
source=out/'layout.cpp';source.write_text('#include "parable_spring.h"\n#include "parable_activation.h"\nextern "C" const uint32_t layout[]={sizeof(parable::dsp::Spring),sizeof(parable::dsp::SpringMode),parable::dsp::Spring::kModes,parable::dsp::Spring::kTaps,sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};\n')
run([compiler,*flags,'-c',source,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o'])
layout=struct.unpack('<7I',(out/'layout.bin').read_bytes());assert layout[0]<=24576 and layout[1:]==(80,192,193,11040,11288,45224),layout
size=run([tool('size'),obj]);text,data,bss=map(int,size.splitlines()[-1].split()[:3]);assert text<=8192 and data==bss==0
report=dict(compiler=pins['toolchain'],flags=flags,object=dict(text=text,data=data,bss=bss),layout=dict(zip(['processor','mode','modes','FIR_taps','unchanged_engine','unchanged_bundle','unchanged_runtime'],layout)),whole_object_undefined=names,audio_calls=audio,stack_usage=(out/'parable_spring.su').read_text().splitlines(),static_work='2 substeps/frame; each fixed damping/kick/rotation/contact/energy loops over192 modes;193-tap output convolution; at most1 contact decision/substep; overload branch scalar sqrt/nextafter; fixed memory zeroing',target_cycles='not measured',future_admission_ceiling_cycles_per_sample=2000,firmware_linked=False,physical_tested=False,source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ['include/parable_spring.h','src/parable_spring.cpp','src/spring_contact.h']},commands=commands)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ('flags','commands')},indent=2))
