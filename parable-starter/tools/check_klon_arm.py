#!/usr/bin/env python3
"""Actual pinned standalone Cortex-M7 Klon object/layout/call audit; no execution."""
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
obj=out/'parable_klon.o';run([compiler,*flags,'-c',root/'src/parable_klon.cpp','-o',obj])
undefined=run([tool('nm'),'-u',obj]);names=sorted(line.split()[-1] for line in undefined.splitlines())
assert set(names)<={'cos','sin','expm1','memcpy','memset'},undefined
assembly=run([tool('objdump'),'-drC',obj]);(out/'klon.disassembly.txt').write_text(assembly);(out/'klon.undefined.txt').write_text(undefined)
headers=list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n',assembly,re.M));audio={}
for i,h in enumerate(headers):
    name=h.group(1)
    selected=any(name.startswith('parable::dsp::Klon::'+f+'(') for f in ['process','sample','solve','evaluate','reconstruct','resetCircuit']) or name.startswith('parable::dsp::klon_detail::')
    if not selected:continue
    body=assembly[h.end():headers[i+1].start() if i+1<len(headers) else len(assembly)]
    calls=re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body)
    assert all(c.startswith(('parable::dsp::Klon::','parable::dsp::klon_detail::')) or c in {'expm1','memcpy','memset'} for c in calls),calls
    assert not any('::'+f+'(' in c for f in ['prepare','matrices','reset'] for c in calls)
    assert not any(s in body for s in ['malloc','operator new','mutex','fopen','__atomic','__sync','throw'])
    audio[name]=calls
for f in ['process','sample','solve','evaluate','reconstruct','resetCircuit']:assert any(n.startswith('parable::dsp::Klon::'+f+'(') for n in audio),f
source=out/'layout.cpp';source.write_text('#include "parable_klon.h"\n#include "parable_activation.h"\n#include "parable_rat.h"\nextern "C" const uint32_t layout[]={sizeof(parable::dsp::Klon),parable::dsp::Klon::kNodes,parable::dsp::Klon::kUnknowns,parable::dsp::Klon::kCaps,parable::dsp::Klon::kFactor,parable::dsp::Klon::kTaps,parable::dsp::Klon::kUpdates,parable::dsp::Klon::kSearch,sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime),sizeof(parable::dsp::Rat)};\n')
run([compiler,*flags,'-c',source,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o'])
layout=struct.unpack('<12I',(out/'layout.bin').read_bytes());assert layout[0]<=12288 and layout[1:]==(21,5,17,12,577,24,8,11040,11288,45224,10752),layout
size=run([tool('size'),obj]);text,data,bss=map(int,size.splitlines()[-1].split()[:3]);assert text<=16384 and data==bss==0
report=dict(compiler=pins['toolchain'],flags=flags,object=dict(text=text,data=data,bss=bss),layout=dict(zip(['processor','nodes','coupled_unknowns','external_caps','oversampling','FIR_taps','Newton_updates','backtracks','unchanged_engine','unchanged_bundle','unchanged_runtime','unchanged_rat'],layout)),whole_object_undefined=names,audio_calls=audio,stack_usage=(out/'parable_klon.su').read_text().splitlines(),
    static_work='12 internal steps/frame;<=24 Newton updates with<=8 backtracking trials+initial evaluation;fixed5x5 elimination;4 exact local amplifier-feedback responses/candidate;2expm1/candidate in BOTH healthy and fault;21nodes/17cap histories;577tap symmetric FIR interpolation/decimation. No approximate exponential/block linearization.',
    maximum_device_evaluations_per_internal_sample=1+24*8,maximum_device_evaluations_per_base_frame=12*(1+24*8),maximum_expm1_per_base_frame=2*12*(1+24*8),
    target_cycles='not measured',future_admission_ceiling_cycles_per_sample=2000,firmware_linked=False,physical_tested=False,
    source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ['include/parable_klon.h','src/parable_klon.cpp','src/klon_device.h']},commands=commands)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['flags','commands']},indent=2))
