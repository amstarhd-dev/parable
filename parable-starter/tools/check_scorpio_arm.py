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
obj=out/'parable_scorpio.o';run([compiler,*flags,'-c',root/'src/parable_scorpio.cpp','-o',obj]);undefined=run([tool('nm'),'-u',obj]);names=sorted(line.split()[-1] for line in undefined.splitlines());assert set(names)<={'cos','sin','expm1','memcpy','memset'},names
assembly=run([tool('objdump'),'-drC',obj]);(out/'scorpio.disassembly.txt').write_text(assembly);(out/'scorpio.undefined.txt').write_text(undefined)
headers=list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n',assembly,re.M));audio={};ops={}
for i,h in enumerate(headers):
    name=h.group(1)
    if not any(name.startswith('parable::dsp::Scorpio::'+f+'(') for f in ['process','sample','value']):continue
    body=assembly[h.end():headers[i+1].start() if i+1<len(headers) else len(assembly)];calls=re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body)
    assert all(c.startswith(('parable::dsp::Scorpio::sample(', 'parable::dsp::Scorpio::value(')) for c in calls),calls
    assert not any(s in body for s in ['expm1','exp','log','malloc','operator new','mutex','fopen','__atomic','__sync','throw'])
    assert '.f64' not in body and '__aeabi_d' not in body, name
    audio[name]=calls;ops[name]={op:len(re.findall(r'\b'+op+r'\.',body)) for op in ['vdiv','vmul','vadd','vsub','vcvt','vmla']}
for f in ['process','sample','value']:assert any(n.startswith('parable::dsp::Scorpio::'+f+'(') for n in audio),f
source=out/'layout.cpp';source.write_text('#include "parable_scorpio.h"\n#include "parable_activation.h"\nextern "C" const uint32_t layout[]={sizeof(parable::dsp::Scorpio),sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};\n')
run([compiler,*flags,'-c',source,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o']);layout=struct.unpack('<4I',(out/'layout.bin').read_bytes());assert layout[0]<=12288 and layout[1:]==(11040,11288,45224),layout
size=run([tool('size'),obj]);text,data,bss=map(int,size.splitlines()[-1].split()[:3]);assert text<=16384 and data==bss==0
report=dict(compiler=pins['toolchain'],flags=flags,object=dict(text=text,data=data,bss=bss),layout=dict(zip(['processor','unchanged_engine','unchanged_bundle','unchanged_runtime'],layout)),whole_object_undefined=names,audio_calls=audio,static_instruction_occurrences=ops,stack_usage=(out/'parable_scorpio.su').read_text().splitlines(),
    work_bound='4 internal steps;97 interpolation+97 decimation products;140 state MACs;at most8 ten-step lookup searches and8 Hermite+primitive evaluations;4 ADAA divisions;12 scalar monotone-cubic Newton updates;C10/flux states;no callback BJT solve/exponential/allocation;lookup8208bytes',
    conditional_cycles_per_frame=2000,estimate_assumptions='Optimistic planning budget only, not an established upper timing bound: cached code/data, fused MACs, amortized loops, no DMA/IRQ stalls. 194 FIR products+140 state MACs, up to8 bounded10-step lookup searches+Hermite/primitive,4 ADAA quotients,12 scalar cubic updates and coupling arithmetic. Need actual M7 cycle counter before admission; budget may be exceeded.',
    target_cycles='not measured',future_admission_ceiling_cycles_per_sample=2000,firmware_linked=False,physical_tested=False,
    source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ['include/parable_scorpio.h','src/parable_scorpio.cpp','src/scorpio_circuit.h']},commands=commands)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['flags','commands']},indent=2))
