#!/usr/bin/env python3
"""Pinned Cortex-M7 object/layout/callback/static frames, never target execution."""
import argparse,hashlib,json,re,struct,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
compiler=a.compiler.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);pins=json.loads((root/'firmware/dependencies.json').read_text());commands=[]
def run(args):
    args=list(map(str,args));commands.append(args);return subprocess.check_output(args,text=True)
assert run([compiler,'-dumpfullversion']).strip()==pins['toolchain']['version']
tool=lambda name:compiler.parent/('arm-none-eabi-'+name)
flags=['-std=c++17','-O2','-Wall','-Wextra','-Wpedantic','-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-fno-exceptions','-fno-rtti','-fno-fast-math','-fno-finite-math-only','-fstack-usage','-I'+str(root/'include')]
obj=out/'parable_wavefolder.o';run([compiler,*flags,'-c',root/'src/parable_wavefolder.cpp','-o',obj]);undefined=run([tool('nm'),'-u',obj]);names=sorted(line.split()[-1] for line in undefined.splitlines());assert set(names)<={'cos','sin','tan','expm1','memcpy','memset'},names
assembly=run([tool('objdump'),'-drC',obj]);(out/'wavefolder.disassembly.txt').write_text(assembly);(out/'wavefolder.undefined.txt').write_text(undefined)
headers=list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n',assembly,re.M));audio={};ops={}
for i,h in enumerate(headers):
    name=h.group(1)
    if not any(name.startswith('parable::dsp::Wavefolder::'+f+'(') for f in ['process','sample','cellCurve']):continue
    body=assembly[h.end():headers[i+1].start() if i+1<len(headers) else len(assembly)];calls=re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body)
    assert all(c.startswith(('parable::dsp::Wavefolder::sample(', 'parable::dsp::Wavefolder::cellCurve(')) for c in calls),calls
    assert not any(s in body for s in ['expm1','exp','log','malloc','operator new','mutex','fopen','__atomic','__sync','throw'])
    audio[name]=calls;ops[name]={op:len(re.findall(r'\b'+op+r'\.',body)) for op in ['vdiv','vmul','vadd','vsub','vcvt','vmla']}
for f in ['process','sample','cellCurve']:assert any(n.startswith('parable::dsp::Wavefolder::'+f+'(') for n in audio),f
source=out/'layout.cpp';source.write_text('#include "parable_wavefolder.h"\n#include "parable_activation.h"\nextern "C" const uint32_t layout[]={sizeof(parable::dsp::Wavefolder),sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};\n')
run([compiler,*flags,'-c',source,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o']);layout=struct.unpack('<4I',(out/'layout.bin').read_bytes());assert layout[0]<=12288 and layout[1:]==(11040,11288,45224),layout
size=run([tool('size'),obj]);text,data,bss=map(int,size.splitlines()[-1].split()[:3]);assert text<=16384 and data==bss==0
report=dict(compiler=pins['toolchain'],flags=flags,object=dict(text=text,data=data,bss=bss),layout=dict(zip(['processor','unchanged_engine','unchanged_bundle','unchanged_runtime'],layout)),whole_object_undefined=names,audio_calls=audio,static_instruction_occurrences=ops,stack_usage=(out/'parable_wavefolder.su').read_text().splitlines(),
    work_bound='4 internal steps;193 interpolation+193 decimation taps;<=8 cubic+primitive lookups,4 ADAA divisions,1 common RC;no callback solver/exponential/allocations;lookup8208bytes',
    conditional_upper_cycles_per_frame=10000,estimate_assumptions='Planning bound only: cached data/code, no IRQ/DMA/memory stalls;386taps*12 +8curves*180 +4divisions*32 +HP/control/index allowance3800=10000. No certified WCET; actual target cycles not measured.',
    target_cycles='not measured',future_admission_ceiling_cycles_per_sample=2000,firmware_linked=False,physical_tested=False,
    source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ['include/parable_wavefolder.h','src/parable_wavefolder.cpp','src/wavefolder_cell.h']},commands=commands)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['flags','commands']},indent=2))
