#!/usr/bin/env python3
"""Pinned full standalone audio-image LINK proof. Never flashes hardware."""
import argparse,gzip,hashlib,json,re,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--libdaisy',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();cc=a.compiler.resolve();sdk=a.libdaisy.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);commands=[]
def run(args,**kwargs):
 args=list(map(str,args));commands.append(args);return subprocess.check_output(args,text=True,**kwargs)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
pins=json.loads((root/'firmware/dependencies.json').read_text());version=run([cc,'--version']);assert 'xPack GNU Arm Embedded GCC' in version;assert run([cc,'-dumpfullversion']).strip()==pins['toolchain']['version'];assert run(['git','-C',sdk,'rev-parse','HEAD']).strip()==pins['libdaisy']['commit']
assert not run(['git','-C',sdk,'status','--porcelain','--untracked-files=no']).strip(),'Pinned SDK has tracked modifications'
submodules={}
for path,want in pins['libdaisy']['submodules'].items():
 got=run(['git','-C',sdk/path,'rev-parse','HEAD']).strip();assert got==want,(path,got,want);submodules[path]=got
archive=sdk/'build/libdaisy.a';assert archive.exists(),'Build pinned libDaisy first with the pinned compiler';ld=sdk/'core/STM32H750IB_flash.lds';ld_sha=sha(ld)
makefile=out/'Makefile';makefile.write_text(f'''TARGET = parable_tiled_room_probe
LIBDAISY_DIR = {sdk}
CPP_SOURCES = {root/'tools/tiled_room_arm_probe.cpp'} {root/'src/parable_tiled_room.cpp'}
C_INCLUDES = -I{root/'include'}
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
CPP_STANDARD = -std=gnu++17
APP_TYPE = BOOT_NONE
OPT = -O2
include $(LIBDAISY_DIR)/core/Makefile
CPPFLAGS += -fno-fast-math -fno-finite-math-only -fstack-usage
CFLAGS += -fstack-usage
''')
cmd=['make','-C',out,'-j2','GCC_PATH='+str(cc.parent),'BUILD_DIR='+str(out/'build')]
commands.append(list(map(str,cmd)));result=subprocess.run(list(map(str,cmd)),capture_output=True,text=True);(out/'build.log').write_text(result.stdout+result.stderr);assert result.returncode==0,'ARM link failed: see build.log';assert sha(ld)==ld_sha,'Linker script changed'
elf=out/'build/parable_tiled_room_probe.elf';tool=lambda n:cc.parent/('arm-none-eabi-'+n)
sections=run([tool('size'),'-A',elf]);symbols=run([tool('nm'),'-SC',elf]);(out/'sections.txt').write_text(sections)
assert 'TiledRoom::process(float)' in symbols,'DSP was not retained in the linked image'
application_symbols={}
for line in symbols.splitlines():
 fields=line.split()
 if len(fields)==4 and fields[3] in ['room','history']:
  application_symbols[fields[3]]=dict(address=int(fields[0],16),bytes=int(fields[1],16))
assert application_symbols['history']['bytes']==12133*4

obj=out/'build/parable_tiled_room.o';size=run([tool('size'),obj]);undefined=run([tool('nm'),'-uC',obj]);assert not any(k in undefined for k in ['operator new','malloc','mutex','fopen','throw','atomic'])
asm=run([tool('objdump'),'-drC',obj]);heads=list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n',asm,re.M));audio={}
for i,h in enumerate(heads):
 if not any(('TiledRoom::'+n+'(') in h.group(1) for n in ['process','tap','push','delay','allpass']):continue
 body=asm[h.end():heads[i+1].start() if i+1<len(heads) else len(asm)];calls=re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body);assert all('TiledRoom::' in c for c in calls),calls;assert '.f64' not in body and '__aeabi_d' not in body
 audio[h.group(1)]=dict(external_calls=calls,float64_instructions=body.count('.f64'))
assert any('TiledRoom::process' in name for name in audio)
sec={}
for line in sections.splitlines():
 fields=line.split()
 if len(fields)>=3 and fields[0].startswith('.'):
  try:sec[fields[0]]=dict(bytes=int(fields[1]),address=int(fields[2]))
  except ValueError:pass
flash=sum(sec.get(n,{}).get('bytes',0) for n in ['.isr_vector','.text','.ARM.extab','.exidx','.init_array','.fini_array','.data']);assert flash<=128*1024
report=dict(elf_sha256=sha(elf),bin_sha256=sha(out/'build/parable_tiled_room_probe.bin'),application_symbols=application_symbols,compiler=version,compiler_sha256=sha(cc),recipe=pins,libdaisy_archive_sha256=sha(archive),libdaisy_submodules=submodules,linker_sha256=ld_sha,layout='official unchanged BOOT_NONE128KiB internal flash; no bootloader migration',flash_load_bytes=flash,sections=sec,native_object=dict(zip(['text','data','bss'],map(int,size.splitlines()[-1].split()[:3]))),native_undefined=undefined,audio_functions=audio,static_frames={p.name:p.read_text().splitlines() for p in (out/'build').glob('*.su')},history_bytes=12133*4,linked=True,hardware_run=False,cycles_measured=False,stack_high_water_measured=False,source_sha256={str(p.relative_to(root)):sha(p) for p in [root/'include/parable_tiled_room.h',root/'src/parable_tiled_room.cpp',root/'tools/tiled_room_arm_probe.cpp']},commands=commands,limits='Actual standalone audio callback links the fault-capable processor,48k only; prepare/runtime hardware operation, analog sound and deadlines untested. Object and static stack evidence are not WCET/high-water.')
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');(out/'symbols.txt.gz').write_bytes(gzip.compress(symbols.encode(),mtime=0));(out/'native-disassembly.txt.gz').write_bytes(gzip.compress(asm.encode(),mtime=0));print(json.dumps({k:report[k] for k in ['flash_load_bytes','native_object','history_bytes','linked','hardware_run']},indent=2))
