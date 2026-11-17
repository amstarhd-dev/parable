#!/usr/bin/env python3
"""Pinned ARM proof of ONLY mixing infrastructure; never full-graph admission."""
import argparse,hashlib,json,re,struct,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();cc=a.compiler.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);commands=[]
def run(args):
 args=list(map(str,args));commands.append(args);return subprocess.check_output(args,text=True)
def tool(s):return cc.parent/('arm-none-eabi-'+s)
pins=json.loads((root/'firmware/dependencies.json').read_text())
version=run([cc,'--version']);assert 'xPack GNU Arm Embedded GCC' in version
assert run([cc,'-dumpfullversion']).strip()==pins['toolchain']['version']
flags=['-std=c++17','-O2','-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-fno-exceptions','-fno-rtti','-fno-fast-math','-fno-finite-math-only','-fstack-usage','-I'+str(root/'tools'),'-I'+str(root/'include')]
obj=out/'routing.o';run([cc,*flags,'-c',root/'tools/wallace_routing.cpp','-o',obj]);asm=run([tool('objdump'),'-drC',obj]);(out/'routing.disassembly.txt').write_text(asm);undefined=run([tool('nm'),'-uC',obj]);assert not any(x in undefined for x in ['malloc','operator new','mutex','fopen','__atomic','__sync','throw'])
heads=list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n',asm,re.M));audio={}
for i,h in enumerate(heads):
 if '::process(' not in h.group(1):continue
 body=asm[h.end():heads[i+1].start() if i+1<len(heads) else len(asm)];calls=re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body);assert all('Highpass::process(' in x or x in ['memset','memcpy'] for x in calls),calls;assert '.f64' not in body and '__aeabi_d' not in body;audio[h.group(1)]={'external_calls':calls,'floating_instructions':{op:len(re.findall(r'\b'+op+r'\.',body)) for op in ['vadd','vsub','vmul','vmla','vmls','vdiv']}}
assert any('Routing::process(' in s for s in audio)
source=out/'layout.cpp';source.write_text('#include "wallace_graph.h"\nextern "C" const unsigned layout[]={sizeof(wallace::Routing),sizeof(parable::dsp::Muff),sizeof(parable::dsp::Rotary),sizeof(parable::dsp::Tape),sizeof(parable::dsp::Hall),sizeof(wallace::Graph),sizeof(wallace::Block)};\n')
run([cc,*flags,'-c',source,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o']);layout=struct.unpack('<7I',(out/'layout.bin').read_bytes());assert layout[0]<512
size=run([tool('size'),obj]);text,data,bss=map(int,size.splitlines()[-1].split()[:3]);assert text<8192 and data==bss==0
report=dict(compiler=version,toolchain_recipe_pin=pins['toolchain'],compiler_binary_sha256=hashlib.sha256(cc.read_bytes()).hexdigest(),routing_object=dict(text=text,data=data,bss=bss),layout=dict(zip(['routing','muff','rotary','tape_metadata','hall','host_graph_shape_on_arm','caller_diagnostic_block'],layout)),tape_medium_bytes=57600*4,callback_functions=audio,whole_object_undefined=undefined,static_frames=(out/'routing.su').read_text().splitlines(),full_graph_linked=False,cycles_measured=False,physical_daisy=False,limitations='Only new routing object compiled/inspected. Graph sizeof probe is not a deployable ARM graph; no SDK/storage placement or full-four-effect link/deadline evidence.',source_sha256={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [root/'tools/wallace_routing.h',root/'tools/wallace_routing.cpp',root/'tools/wallace_graph.h']},commands=commands)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='commands'},indent=2))
