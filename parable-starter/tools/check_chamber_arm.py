#!/usr/bin/env python3
"""Pinned selected whole-core M7 compile/layout/static-call audit, never admission."""
import argparse,hashlib,json,re,struct,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
compiler=a.compiler.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);commands=[]
def run(args):
 args=list(map(str,args));commands.append(args);return subprocess.check_output(args,text=True,stderr=subprocess.STDOUT)
pin=json.loads((root/'firmware/dependencies.json').read_text())['toolchain'];assert run([compiler,'-dumpfullversion']).strip()==pin['version']
tool=lambda n:compiler.parent/('arm-none-eabi-'+n)
flags=['-std=c++17','-O2','-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-fno-exceptions','-fno-rtti','-fno-fast-math','-fno-finite-math-only','-fstack-usage','-DBUFFER_SIZE=64','-DMAX_STR_SIZE=32','-include',str(root/'tools/chamber_cloud_portability.h'),'-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(root/'third_party/chamber/cloudseed')]
sources=['src/parable_chamber.cpp','third_party/chamber/cloudseed/DSP/Biquad.cpp','third_party/chamber/cloudseed/DSP/RandomBuffer.cpp'];objects=[];bodies={};frames={};sizes={};undefined={}
for source in sources:
 name=Path(source).stem;obj=out/(name+'.o');objects.append(obj);log=run([compiler,*flags,'-c',root/source,'-o',obj]);(out/(name+'-compile.log')).write_text(log)
 text,data,bss=map(int,run([tool('size'),obj]).splitlines()[-1].split()[:3]);sizes[name]=dict(text=text,data=data,bss=bss)
 undef=run([tool('nm'),'-u',obj]);undefined[name]=sorted(line.split()[-1] for line in undef.splitlines())
 assembly=run([tool('objdump'),'-drC',obj]);(out/(name+'.disassembly.txt')).write_text(assembly)
 headers=list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n',assembly,re.M))
 for i,h in enumerate(headers):bodies[h.group(1)]=assembly[h.end():headers[i+1].start() if i+1<len(headers)else len(assembly)]
 for line in (out/(name+'.su')).read_text().splitlines():
  parts=line.split('\t');function=parts[0].split(':',3)[-1];frames[function]=int(parts[1])
starts=[name for name in bodies if name.startswith(('parable::soundforge::Chamber::process(', 'parable::soundforge::Chamber::setReturnGain('))];assert len(starts)==2
permitted={'memcpy','memset','sinf','expf','sqrt','sqrtf','fmod','tanhf','powf'};reachable={};pending=starts[:]
while pending:
 name=pending.pop()
 if name in reachable:continue
 body=bodies[name];calls=re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body)
 assert not re.search(r'\bblx?\s+(?:r\d+|ip)\b|\bbx\s+(?:r\d+|ip)\b',body),name
 for call in calls:
  if call in bodies:pending.append(call)
  else:assert call in permitted,(name,call)
 assert not any(word in '\n'.join(calls) for word in ['atomic','mutex','malloc','operator new','throw','fopen','printf']),name
 reachable[name]=calls
layout=out/'layout.cpp';layout.write_text('#include "parable_chamber.h"\n#include "DSP/ReverbController.h"\n#include "parable_activation.h"\nextern "C" const uint32_t layout[]={sizeof(parable::soundforge::Chamber),sizeof(Cloudseed::ReverbController),sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};\n')
run([compiler,*flags,'-c',layout,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o']);values=struct.unpack('<5I',(out/'layout.bin').read_bytes());assert values[2:]==(11040,11288,45224)
report=dict(toolchain=pin,flags=flags,objects=sizes,whole_objects_undefined=undefined,layout=dict(zip(['wrapper','borrowed_core','unchanged_engine','unchanged_bundle','unchanged_runtime'],values)),delay_buffer_bytes_each_rate={'44100':66981888,'48000':66981888,'96000':66981888},audio_calls=reachable,static_frames=frames,
 memory_note='67MB inline core plus live upstream seed-vector allocations, wrapper and allocator overhead. No flash/SDRAM placement or hardware admission. Fixed upstream capacities retained.',
 work_bound='Exactly64frames/block; up to12late lines/channel and bounded12stage diffusers;8sample modulation update; one common bounded amplifier/frame; no audio heap/IO/locks/atomics found. libm/math/helper stack and cycles not measured.',
 firmware_linked=False,physical_tested=False,target_cycles='not measured',commands=commands,source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in sources+['include/parable_chamber.h','src/chamber_cloud_settings.h','third_party/chamber/initialization.patch']})
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
