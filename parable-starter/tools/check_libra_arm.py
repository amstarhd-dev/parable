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
obj=out/'parable_libra.o';run([compiler,*flags,'-c',root/'src/parable_libra.cpp','-o',obj]);undefined=run([tool('nm'),'-u',obj]);names=sorted(line.split()[-1] for line in undefined.splitlines());assert set(names)<={'cos','sin','pow','memcpy','memset'},names
assembly=run([tool('objdump'),'-drC',obj]);(out/'libra.disassembly.txt').write_text(assembly);(out/'libra.undefined.txt').write_text(undefined)
headers=list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n',assembly,re.M));audio={};ops={}
for i,h in enumerate(headers):
    name=h.group(1)
    if not any(name.startswith('parable::dsp::Libra::'+f+'(') for f in ['process','sample','select']):continue
    body=assembly[h.end():headers[i+1].start() if i+1<len(headers) else len(assembly)];calls=re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body)
    assert all(c.startswith(('parable::dsp::Libra::sample(', 'parable::dsp::Libra::select(')) or c=='memset' for c in calls),calls
    lines=body.splitlines()
    for n,line in enumerate(lines):
        if re.search(r'\bbl(?:x|\.w)?\s',line):assert n+1<len(lines) and re.search(r'R_ARM_THM_(?:CALL|JUMP24)',lines[n+1]),'Unresolved direct/indirect callback call'
    if 'memset' in calls:
        assert name=='parable::dsp::Libra::select(float, bool) const'
        sites=[n for n,line in enumerate(lines) if 'R_ARM_THM_CALL\tmemset' in line]
        assert len(sites)==1 and re.search(r'movs\s+r2, #32',lines[sites[0]-4]) and re.search(r'movs\s+r1, #0',lines[sites[0]-3]),'Only fixed32-byte zero fill allowed'
    assert not any(s in body for s in ['expm1','exp','log','malloc','operator new','mutex','fopen','__atomic','__sync','throw'])
    assert '.f64' not in body and '__aeabi_d' not in body, name
    audio[name]=calls;ops[name]={op:len(re.findall(r'\b'+op+r'\.',body)) for op in ['vdiv','vmul','vadd','vsub','vcvt','vmla']}
for f in ['process','sample','select']:assert any(n.startswith('parable::dsp::Libra::'+f+'(') for n in audio),f
source=out/'layout.cpp';source.write_text('#include "parable_libra.h"\n#include "parable_activation.h"\nextern "C" const uint32_t layout[]={sizeof(parable::dsp::Libra),sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};\n')
run([compiler,*flags,'-c',source,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o']);layout=struct.unpack('<4I',(out/'layout.bin').read_bytes());assert layout[0]<=2048 and layout[1:]==(11040,11288,45224),layout
size=run([tool('size'),obj]);text,data,bss=map(int,size.splitlines()[-1].split()[:3]);assert text<=16384 and data==bss==0
report=dict(compiler=pins['toolchain'],flags=flags,object=dict(text=text,data=data,bss=bss),layout=dict(zip(['processor','unchanged_engine','unchanged_bundle','unchanged_runtime'],layout)),whole_object_undefined=names,audio_calls=audio,static_instruction_occurrences=ops,stack_usage=(out/'parable_libra.su').read_text().splitlines(),
    work_bound='4 internal steps,at most4 affine-region candidates each;194 FIR products/base frame and fixed RC/branch-current arithmetic;no iterative junction solve/table search/runtime libm. Error-only memset has compiled constant32-byte size.',
    target_cycles='not measured; no cycle budget asserted',firmware_linked=False,physical_tested=False,
    source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ['include/parable_libra.h','src/parable_libra.cpp']},commands=commands)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['flags','commands']},indent=2))
