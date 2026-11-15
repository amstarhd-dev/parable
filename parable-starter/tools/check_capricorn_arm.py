#!/usr/bin/env python3
"""SF018 specialization of main's existing M7 object/layout/call audit, no execution."""
import argparse,hashlib,json,re,struct,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();compiler=a.compiler.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);pins=json.loads((root/'firmware/dependencies.json').read_text());commands=[]
def run(args):
    args=list(map(str,args));commands.append(args);return subprocess.check_output(args,text=True)
assert run([compiler,'-dumpfullversion']).strip()==pins['toolchain']['version']
tool=lambda n:compiler.parent/('arm-none-eabi-'+n)
flags=['-std=c++17','-O2','-Wall','-Wextra','-Wpedantic','-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-fno-exceptions','-fno-rtti','-fno-fast-math','-fno-finite-math-only','-fstack-usage','-I'+str(root/'include')]
obj=out/'parable_capricorn.o';run([compiler,*flags,'-c',root/'src/parable_capricorn.cpp','-o',obj]);undefined=run([tool('nm'),'-u',obj]);names=sorted(s.split()[-1] for s in undefined.splitlines());assert set(names)<={'cos','sin','expm1','sinh','cosh','memcpy','memset'},names
assembly=run([tool('objdump'),'-drC',obj]);(out/'capricorn.disassembly.txt').write_text(assembly);(out/'capricorn.undefined.txt').write_text(undefined)
headers=list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n',assembly,re.M));functions={h.group(1):assembly[h.end():headers[i+1].start() if i+1<len(headers) else len(assembly)] for i,h in enumerate(headers)}
def calls(body):
    result=set(re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body));lines=body.splitlines()
    for i,line in enumerate(lines):
        m=re.search(r'\bbl(?:x|\.w)?\s+[^<]*<(.+)>',line)
        if m and not (i+1<len(lines) and re.search(r'R_ARM_THM_(?:CALL|JUMP24)',lines[i+1])):result.add(re.sub(r'\+0x[0-9a-f]+$','',m.group(1)))
        assert not re.search(r'\bblx\s+r\d+',line),'Indirect audio call'
    return sorted(result)
audio={};pending=['parable::dsp::Capricorn::process(float)']
while pending:
    name=pending.pop()
    if name in audio:continue
    assert name in functions,name
    body=functions[name];assert not any(x in body for x in ['.f64','__aeabi_d','expm1','sinh','cosh','malloc','operator new','mutex','fopen','__atomic','__sync','throw']),name
    audio[name]=calls(body)
    for target in audio[name]:
        assert target in functions,(name,target)
        pending.append(target)
source=out/'layout.cpp';source.write_text('#include "parable_capricorn.h"\n#include "parable_activation.h"\nextern "C" const uint32_t layout[]={sizeof(parable::dsp::Capricorn),sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};\n')
run([compiler,*flags,'-c',source,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o']);layout=struct.unpack('<4I',(out/'layout.bin').read_bytes());assert layout[0]<=10240 and layout[1:]==(11040,11288,45224),layout
size=run([tool('size'),obj]);text,data,bss=map(int,size.splitlines()[-1].split()[:3]);assert data==bss==0
report=dict(compiler=pins['toolchain'],flags=flags,object=dict(text=text,data=data,bss=bss),layout=dict(zip(['processor','unchanged_engine','unchanged_bundle','unchanged_runtime'],layout)),whole_object_undefined=names,audio_calls=audio,stack_usage=(out/'parable_capricorn.su').read_text().splitlines(),work_bound='8 internal steps;193 interpolation+193 decimation products;<=376 companion MACs plus six RCs and curve arithmetic/internal;<=24 nine-step Hermite lookups incl Q2 primitive midpoint branch,8 ADAA quotients. No callback transistor/diode solve or exponential.',target_cycles='not measured; no timing estimate asserted',firmware_linked=False,physical_tested=False,source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ['include/parable_capricorn.h','src/parable_capricorn.cpp','src/capricorn_circuit.h']},commands=commands)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['flags','commands','stack_usage']},indent=2))
