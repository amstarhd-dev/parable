#!/usr/bin/env python3
"""Pinned actual M7 object/layout/call audit. This does not execute on a Daisy."""
import argparse,hashlib,json,re,struct,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
compiler=a.compiler.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
pins=json.loads((root/'firmware/dependencies.json').read_text());commands=[]
def run(args):
    args=list(map(str,args));commands.append(args);return subprocess.check_output(args,text=True)
assert run([compiler,'-dumpfullversion']).strip()==pins['toolchain']['version']
tool=lambda name:compiler.parent/('arm-none-eabi-'+name)
flags=['-std=c++17','-O2','-Wall','-Wextra','-Wpedantic','-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-fno-exceptions','-fno-rtti','-fno-fast-math','-fno-finite-math-only','-fstack-usage','-I'+str(root/'include')]
obj=out/'parable_tape_saturation.o';run([compiler,*flags,'-c',root/'src/parable_tape_saturation.cpp','-o',obj])
undefined=run([tool('nm'),'-u',obj]);names=sorted(line.split()[-1] for line in undefined.splitlines())
assert set(names)<={'atan','cos','sin','tan','exp','expm1','erf','memcpy','memset','memmove'},undefined
assembly=run([tool('objdump'),'-drC',obj]);(out/'tape-saturation.disassembly.txt').write_text(assembly);(out/'tape-saturation.undefined.txt').write_text(undefined)
headers=list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n',assembly,re.M));audio={}
for i,h in enumerate(headers):
    name=h.group(1)
    if not any(name.startswith('parable::dsp::TapeSaturation::'+f+'(') for f in ['process','material','parcel','materialReference']):continue
    body=assembly[h.end():headers[i+1].start() if i+1<len(headers) else len(assembly)]
    calls=re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body)
    assert all(c.startswith(('parable::dsp::TapeSaturation::parcel(','parable::dsp::TapeSaturation::material(')) or c in {'memcpy','memset'} for c in calls),calls
    assert not any(s in body for s in ['malloc','operator new','mutex','fopen','__atomic','__sync','throw'])
    audio[name]=calls
for f in ['process','material','parcel','materialReference']:assert any(n.startswith('parable::dsp::TapeSaturation::'+f+'(') for n in audio),f
source=out/'layout.cpp';source.write_text('#include "parable_tape_saturation.h"\n#include "parable_activation.h"\n#include "parable_rat.h"\nextern "C" const uint32_t layout[]={sizeof(parable::dsp::TapeSaturation),parable::dsp::TapeSaturation::kRows,parable::dsp::TapeSaturation::kPhases,parable::dsp::TapeSaturation::kEvents,parable::dsp::TapeSaturation::kHistory,parable::dsp::TapeSaturation::kFactor,parable::dsp::TapeSaturation::kTaps,parable::dsp::TapeSaturation::kHeadTaps,sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime),sizeof(parable::dsp::Rat)};\n')
run([compiler,*flags,'-c',source,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o'])
layout=struct.unpack('<12I',(out/'layout.bin').read_bytes());assert layout[0]<=16384 and layout[1:]==(8,8,48,256,4,193,257,11040,11288,45224,10752),layout
size=run([tool('size'),obj]);text,data,bss=map(int,size.splitlines()[-1].split()[:3]);assert text<=24576 and data==bss==0
report=dict(compiler=pins['toolchain'],flags=flags,object=dict(text=text,data=data,bss=bss),layout=dict(zip(['processor','rows','phases','event_capacity','history','oversampling','FIR_taps','pickup_taps','unchanged_engine','unchanged_bundle','unchanged_runtime','unchanged_rat'],layout)),whole_object_undefined=names,audio_calls=audio,stack_usage=(out/'parable_tape_saturation.su').read_text().splitlines(),
    static_work='4 internal steps/frame;8 phases each<=48 prepared events;8 positive coercivity rows/event;no iterative solver;<=12288 boundary updates and256 Gaussian cubic evaluations/frame;193tap interpolation+decimation;257tap pickup. All atan/exp/erf/cos/sin/tan confined to stopped prepare.',
    maximum_boundary_updates_per_frame=4*8*48*8,maximum_cdf_evaluations_per_frame=4*8*8,target_cycles='not measured',future_admission_ceiling_cycles_per_sample=2000,firmware_linked=False,physical_tested=False,
    source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ['include/parable_tape_saturation.h','src/parable_tape_saturation.cpp','src/tape_saturation_material.h']},commands=commands)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['flags','commands']},indent=2))
