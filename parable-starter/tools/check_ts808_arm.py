#!/usr/bin/env python3
"""Actual pinned standalone Cortex-M7 TS808 object/layout/call audit; no execution."""
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
obj=out/'parable_ts808.o';run([compiler,*flags,'-c',root/'src/parable_ts808.cpp','-o',obj])
undefined=run([tool('nm'),'-u',obj]);names=sorted(line.split()[-1] for line in undefined.splitlines())
assert set(names)<={'cos','sin','pow','expm1','memcpy','memset'},undefined
assembly=run([tool('objdump'),'-drC',obj]);(out/'ts808.disassembly.txt').write_text(assembly);(out/'ts808.undefined.txt').write_text(undefined)
headers=list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n',assembly,re.M));audio={}
for i,h in enumerate(headers):
    name=h.group(1)
    selected=any(name.startswith('parable::dsp::Ts808::'+f+'(') for f in ['process','sample','solve','evaluate','reconstruct','resetCircuit']) or name.startswith('parable::dsp::ts808_detail::')
    if not selected:continue
    body=assembly[h.end():headers[i+1].start() if i+1<len(headers) else len(assembly)]
    calls=re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body)
    assert all(c.startswith(('parable::dsp::Ts808::','parable::dsp::ts808_detail::')) or c in {'expm1','memcpy','memset'} for c in calls),calls
    assert not any('::'+f+'(' in c for f in ['prepare','matrices','reset'] for c in calls)
    assert not any(s in body for s in ['malloc','operator new','mutex','fopen','__atomic','__sync','throw'])
    audio[name]=calls
for f in ['process','sample','solve','evaluate','reconstruct','resetCircuit']:assert any(n.startswith('parable::dsp::Ts808::'+f+'(') for n in audio),f
source=out/'layout.cpp';source.write_text('#include "parable_ts808.h"\n#include "parable_activation.h"\n#include "parable_rat.h"\nextern "C" const uint32_t layout[]={sizeof(parable::dsp::Ts808),parable::dsp::Ts808::kNodes,parable::dsp::Ts808::kJunctions,parable::dsp::Ts808::kCaps,parable::dsp::Ts808::kFactor,parable::dsp::Ts808::kTaps,parable::dsp::Ts808::kUpdates,parable::dsp::Ts808::kSearch,sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime),sizeof(parable::dsp::Rat)};\n')
run([compiler,*flags,'-c',source,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o'])
layout=struct.unpack('<12I',(out/'layout.bin').read_bytes());assert layout[0]<=8192 and layout[1:]==(12,5,9,16,769,32,12,11040,11288,45224,10752),layout
size=run([tool('size'),obj]);text,data,bss=map(int,size.splitlines()[-1].split()[:3]);assert text<=16384 and data==bss==0
report=dict(compiler=pins['toolchain'],flags=flags,object=dict(text=text,data=data,bss=bss),layout=dict(zip(['processor','nodes','nonlinear_junctions','external_caps','oversampling','FIR_taps','Newton_updates','backtracks','unchanged_engine','unchanged_bundle','unchanged_runtime','unchanged_rat'],layout)),whole_object_undefined=names,audio_calls=audio,stack_usage=(out/'parable_ts808.su').read_text().splitlines(),
    static_work='16 internal steps/frame; each <=32 Newton updates with<=12 backtracking trials plus initial evaluation; fixed5x5 elimination;<=25 paired affine2x2 amplifier regions/device evaluation;6expm1 healthy or5fault;12nodes/9cap histories;769tap symmetric FIR interpolation/decimation. Exact amplifier elimination, no approximate exponential or block linearization.',
    maximum_device_evaluations_per_internal_sample=1+32*12,maximum_device_evaluations_per_base_frame=16*(1+32*12),maximum_expm1_per_base_frame=6*16*(1+32*12),
    target_cycles='not measured',future_admission_ceiling_cycles_per_sample=2000,firmware_linked=False,physical_tested=False,
    source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ['include/parable_ts808.h','src/parable_ts808.cpp','src/ts808_device.h']},commands=commands)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['flags','commands']},indent=2))
