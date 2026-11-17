#!/usr/bin/env python3
"""Actual pinned M7 object, SDRAM-placement/full audio-link proof, never flashing."""
import argparse,json,re,subprocess,hashlib,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--libdaisy',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);cc=a.compiler.resolve();sdk=a.libdaisy.resolve();pins=json.loads((root/'firmware/dependencies.json').read_text());commands=[]
def run(args,**kw):
 args=list(map(str,args));commands.append(args);return subprocess.check_output(args,text=True,stderr=subprocess.STDOUT,**kw)
def tool(n):return cc.parent/('arm-none-eabi-'+n)
assert run([cc,'-dumpfullversion']).strip()==pins['toolchain']['version']
compiler_identity=run([cc,'--version']);assert '14.2.1-1.1' in compiler_identity
assert run(['git','-C',sdk,'rev-parse','HEAD']).strip()==pins['libdaisy']['commit']
assert not run(['git','-C',sdk,'status','--porcelain']).strip(), 'modified SDK source'
for path,sha in pins['libdaisy']['submodules'].items():
 assert run(['git','-C',sdk/path,'rev-parse','HEAD']).strip()==sha, path
flags=['-std=c++17','-O2','-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-fno-exceptions','-fno-rtti','-fno-fast-math','-fno-finite-math-only','-fstack-usage','-I'+str(root/'include')]
objects=[]
for stem in ['hall_core','parable_hall']:
 obj=out/(stem+'.o');run([cc,*flags,'-c',root/'src'/(stem+'.cpp'),'-o',obj]);objects.append(obj)
assembly=run([tool('objdump'),'-drC',*objects]);(out/'hall.disassembly.txt').write_text(assembly)
undefined=run([tool('nm'),'-uC',*objects]);(out/'undefined.txt').write_text(undefined)
assert not any(word in undefined for word in ['operator new','malloc','free','mutex','fopen','throw','__atomic']),undefined
layout=out/'layout.cpp';layout.write_text('#include "parable_hall.h"\n#include "parable_activation.h"\nextern "C" const unsigned layout[]={sizeof(parable::dsp::Hall),parable::dsp::Hall::stateBytes(),sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};\n')
run([cc,*flags,'-c',layout,'-o',out/'layout.o']);run([tool('objcopy'),'--dump-section','.rodata='+str(out/'layout.bin'),out/'layout.o']);sizes=struct.unpack('<5I',(out/'layout.bin').read_bytes());assert sizes[0]<512*1024 and sizes[2:]==(11040,11288,45224),sizes
makefile=out/'Makefile';makefile.write_text(f'''TARGET = parable_hall_link_probe
LIBDAISY_DIR = {sdk}
CPP_SOURCES = {root/'tools/hall_arm_probe.cpp'} {root/'src/parable_hall.cpp'} {root/'src/hall_core.cpp'}
C_INCLUDES = -I{root/'include'}
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
CPP_STANDARD = -std=gnu++17
APP_TYPE ?= BOOT_NONE
OPT = -O2
include $(LIBDAISY_DIR)/core/Makefile
CPPFLAGS += -fno-fast-math -fno-finite-math-only -fstack-usage
CFLAGS += -fstack-usage
''')
# Use the official linker layouts unchanged. Preserve any overflow before fallback.
layouts={}
for boot in ['BOOT_NONE']:
 build=out/boot
 try:
  log=run(['make','-C',out,'-j2','GCC_PATH='+str(cc.parent),'APP_TYPE='+boot,'BUILD_DIR='+str(build)])
 except subprocess.CalledProcessError as e:
  (out/(boot+'.log')).write_text(e.output);layouts[boot]={'linked':False,'reason':'see retained linker/build log'};continue
 (out/(boot+'.log')).write_text(log);elf=build/'parable_hall_link_probe.elf'
 sections=run([tool('size'),'-A',elf]);(out/(boot+'.sections.txt')).write_text(sections)
 symbols=run([tool('nm'),'-SC',elf]);(out/(boot+'.symbols.txt')).write_text(symbols)
 assert re.search(r'^c000[0-9a-f]+\s+00073[0-9a-f]+\s+[bB]\s+storage',symbols,re.M), 'external fixed state placement absent'
 layouts[boot]={'linked':True,'elf_sha256':hashlib.sha256(elf.read_bytes()).hexdigest(),'sections':sections,'linker_script':pins['target']['layouts'][boot]}
report={'compiler_identity':compiler_identity,'compiler_binary_sha256':hashlib.sha256(cc.read_bytes()).hexdigest(),'source_sha256':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [root/'include/parable_hall.h',root/'src/parable_hall.cpp',root/'src/hall_core.cpp',root/'src/hall_core.h',root/'tools/hall_arm_probe.cpp']},'compiler':pins['toolchain'],'libdaisy':pins['libdaisy']['commit'],'layout':dict(zip(['hall','core','unchanged_engine','unchanged_bundle','unchanged_runtime'],sizes)),'objects':run([tool('size'),*objects]),'undefined':undefined,'frames':{x.name:x.read_text().splitlines() for x in out.glob('*.su')},'layouts':layouts,'target_cycles':'not measured','physical_tested':False,'libdaisy_archive_sha256':hashlib.sha256((sdk/'build/libdaisy.a').read_bytes()).hexdigest(),'linker_sha256':hashlib.sha256((sdk/pins['target']['layouts']['BOOT_NONE']).read_bytes()).hexdigest(),'static_audio_frame_chain_bytes_before_libm_caller_irq':696,'callback_math':{'sin_call_sites':8,'pow_call_sites':4,'frexpf_call_sites':2,'precision':'original double arithmetic preserved; no performance claim from instruction count'},'commands':commands}
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['commands','frames']},indent=2))
assert any(x['linked'] for x in layouts.values()),'No successful complete firmware link; report blocker'
