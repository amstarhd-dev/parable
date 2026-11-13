#!/usr/bin/env python3
"""Pinned actual standalone M7 Muff object/layout/audio-call audit, no execution."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
compiler=a.compiler.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);pins=json.loads((root/'firmware/dependencies.json').read_text());commands=[]
def run(args):
    args=list(map(str,args));commands.append(args);return subprocess.check_output(args,text=True)
assert run([compiler,'-dumpfullversion']).strip()==pins['toolchain']['version']
tool=lambda n:compiler.parent/('arm-none-eabi-'+n)
flags=['-std=c++17','-O2','-Wall','-Wextra','-Wpedantic','-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-fno-exceptions','-fno-rtti','-fno-fast-math','-fno-finite-math-only','-fstack-usage','-I'+str(root/'include')]
obj=out/'parable_muff.o';run([compiler,*flags,'-c',root/'src/parable_muff.cpp','-o',obj]);undefined=run([tool('nm'),'-u',obj]);names=sorted(l.split()[-1] for l in undefined.splitlines());assert set(names)<={'cos','sin','expm1','memcpy','memset'},undefined
assembly=run([tool('objdump'),'-drC',obj]);(out/'muff.disassembly.txt').write_text(assembly);(out/'muff.undefined.txt').write_text(undefined)
headers=list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n',assembly,re.M));audio={}
for i,h in enumerate(headers):
    name=h.group(1)
    if not any(name.startswith('parable::dsp::Muff::'+f+'(') for f in ['process','sample','solve','reconstruct','resetCircuit']) and not name.startswith('parable::dsp::muff_detail::currents('):continue
    body=assembly[h.end():headers[i+1].start() if i+1<len(headers) else len(assembly)]
    calls=re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body)
    assert all(c.startswith(('parable::dsp::Muff::','parable::dsp::muff_detail::currents(')) or c in {'expm1','memcpy','memset'} for c in calls),calls
    assert not any('::'+f+'(' in c for f in ['prepare','matrices'] for c in calls)
    assert not any(s in body for s in ['malloc','operator new','mutex','fopen','__atomic','__sync','throw'])
    audio[name]=calls
assert len(audio)==6,audio
source=out/'layout.cpp';source.write_text('#include "parable_muff.h"\n#include "parable_activation.h"\nextern "C" const uint32_t layout[]={sizeof(parable::dsp::Muff),parable::dsp::Muff::kNodes,parable::dsp::Muff::kPorts,parable::dsp::Muff::kCaps,parable::dsp::Muff::kUpdates,parable::dsp::Muff::kTaps,sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};\n')
run([compiler,*flags,'-c',source,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o']);layout=struct.unpack('<9I',(out/'layout.bin').read_bytes());assert layout[0]<=16384 and layout[1:]==(17,10,13,24,193,11040,11288,45224),layout
size=run([tool('size'),obj]);text,data,bss=map(int,size.splitlines()[-1].split()[:3]);assert text<=16384 and data==bss==0
report=dict(compiler=pins['toolchain'],flags=flags,object=dict(text=text,data=data,bss=bss),layout=dict(zip(['processor','nodes','ports','capacitors','Newton_updates','FIR_taps','unchanged_engine','unchanged_bundle','unchanged_runtime'],layout)),whole_object_undefined=names,audio_calls=audio,stack_usage=(out/'parable_muff.su').read_text().splitlines(),static_work='4 internal steps/frame; each <=24 Newton updates+final evaluation, fixed10x10 pivoted elimination;12 expm1 calls/evaluation (8 BJT+4 diode);17-node reconstruction/13 capacitor histories;193-tap FIR interpolation+decimation; no approximate math/block linearization',maximum_device_evaluations_per_base_frame=100,maximum_expm1_per_base_frame=1200,target_cycles='not measured',future_admission_ceiling_cycles_per_sample=2000,firmware_linked=False,physical_tested=False,source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ['include/parable_muff.h','src/parable_muff.cpp','src/muff_device.h']},commands=commands)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['flags','commands']},indent=2))
