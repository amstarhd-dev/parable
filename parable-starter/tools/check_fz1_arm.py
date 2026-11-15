#!/usr/bin/env python3
"""Pinned M7 object/layout/static frames and all relocations; not target timing."""
import argparse,hashlib,json,re,struct,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();compiler=a.compiler.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);pins=json.loads((root/'firmware/dependencies.json').read_text());commands=[]
def run(args):
    args=list(map(str,args));commands.append(args);return subprocess.check_output(args,text=True)
assert run([compiler,'-dumpfullversion']).strip()==pins['toolchain']['version']
tool=lambda name:compiler.parent/('arm-none-eabi-'+name)
flags=['-std=c++17','-O2','-Wall','-Wextra','-Wpedantic','-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-fno-exceptions','-fno-rtti','-fno-fast-math','-fno-finite-math-only','-fstack-usage','-I'+str(root/'include')]
obj=out/'parable_fz1.o';run([compiler,*flags,'-c',root/'src/parable_fz1.cpp','-o',obj]);undefined=run([tool('nm'),'-u',obj]);names=sorted(line.split()[-1] for line in undefined.splitlines());assert set(names)<={'cos','sin','expm1','sqrt','memcpy','memset'},names
assembly=run([tool('objdump'),'-drC',obj]);(out/'fz1.disassembly.txt').write_text(assembly);(out/'fz1.undefined.txt').write_text(undefined)
headers=list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n',assembly,re.M));calls={};static_ops={}
for i,h in enumerate(headers):
    body=assembly[h.end():headers[i+1].start() if i+1<len(headers) else len(assembly)]
    targets=[];lines=body.splitlines()
    for j,line in enumerate(lines):
        assert not re.search(r'\bblx\s',line),'Unexpected indirect call: '+line
        match=re.search(r'\b(?:bl|b\.w)\s+[0-9a-f]+ <(.+)>',line)
        if not match:continue
        reloc=re.search(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',lines[j+1] if j+1<len(lines) else '')
        target=reloc.group(1) if reloc else re.sub(r'\+0x[0-9a-f]+$','',match.group(1))
        if target!=h.group(1):targets.append(target)
    calls[h.group(1)]=targets
    static_ops[h.group(1)]={op:len(re.findall(r"\b"+op+r"\.",body)) for op in ["vsqrt","vdiv","vmul","vmla","vadd","vsub","vcvt"]}
    assert not any(x in body for x in ['malloc','operator new','pthread','fopen','__atomic','__sync','__cxa_throw'])
start=next(n for n in calls if n.startswith('parable::dsp::Fz1::process('));reachable=set();pending=[start]
while pending:
    name=pending.pop()
    if name in reachable:continue
    reachable.add(name);pending.extend(calls.get(name,[]))
assert 'expm1' in reachable and 'sqrt' in reachable
assert not any(n in reachable for n in ['cos','sin'])
assert all(n in calls or n in {'expm1','sqrt','memcpy','memset'} for n in reachable),reachable
source=out/'layout.cpp';source.write_text('#include "parable_fz1.h"\n#include "parable_activation.h"\nextern "C" const uint32_t layout[]={sizeof(parable::dsp::Fz1),sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};\n')
run([compiler,*flags,'-c',source,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o']);layout=struct.unpack('<4I',(out/'layout.bin').read_bytes());assert layout[0]<=12288 and layout[1:]==(11040,11288,45224),layout
size=run([tool('size'),obj]);text,data,bss=map(int,size.splitlines()[-1].split()[:3]);assert text<=16384 and data==bss==0
report=dict(compiler=pins['toolchain'],flags=flags,object=dict(text=text,data=data,bss=bss),layout=dict(zip(['processor','unchanged_engine','unchanged_bundle','unchanged_runtime'],layout)),whole_object_undefined=names,calls=calls,callback_reachable=sorted(reachable),static_callback_instruction_occurrences={n:static_ops[n] for n in sorted(reachable) if n in static_ops},instruction_count_note="Static disassembly sites only, not runtime counts or cycle estimates",stack_usage=(out/'parable_fz1.su').read_text().splitlines(),
 work_bound='8 internal samples, <=24 Newton updates each, <=8 bounded domain retries/update, at most193 device evaluations/internal sample. Each evaluation3GP=12expm1+3sqrt; typical measured host counts separately. Finite loops, FIR385taps in/out; no cache/block linearization.',
 target_cycles='not measured; no calibrated math-cycle or WCET estimate supplied',static_frames='compiler local frames only, not measured stack high-water; transitive libm frames not included',future_admission_ceiling_cycles_per_sample=2000,firmware_linked=False,physical_tested=False,
 source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ['include/parable_fz1.h','src/parable_fz1.cpp','src/germanium_device.h']},commands=commands)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['flags','commands','calls']},indent=2))
