#!/usr/bin/env python3
"""Build three RAM-only startup diagnostics, retaining original v0.1 evidence."""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
def sha(b):return hashlib.sha256(b).hexdigest()
def run(args):return subprocess.check_output(list(map(str,args)),stderr=subprocess.STDOUT).decode()
def require(ok,message):
 if not ok:raise RuntimeError(message)
def build(compiler,sdk,out):
 pins=json.loads((ROOT/'firmware/dependencies.json').read_text())
 require(run([compiler,'-dumpfullversion']).strip()==pins['toolchain']['version'] and 'xpack' in run([compiler,'--version']).lower(),'Wrong compiler')
 require(run(['git','-C',sdk,'rev-parse','HEAD']).strip()==pins['libdaisy']['commit'],'Wrong SDK')
 require(not run(['git','-C',sdk,'status','--porcelain','--untracked-files=no']).strip(),'Modified SDK')
 for path,commit in pins['libdaisy']['submodules'].items():require(run(['git','-C',sdk/path,'rev-parse','HEAD']).strip()==commit,'Wrong submodule')
 sources={p.name:sha(p.read_bytes()) for p in [HERE/'entry.S',HERE/'diagnostic.cpp',HERE/'diagnostic.ld',HERE/'build.py',HERE.parent/'ram_identity/protocol.h']}
 out.mkdir(parents=True,exist_ok=True);images=[]
 for mode,name in enumerate(['entry','startup','reset']):
  d=out/name;d.mkdir(exist_ok=True)
  config=dict(schema='parable.ram-startup-build.v1',mode=mode,pins=pins,sources=sources,compiler_sha256=sha(compiler.read_bytes()))
  identity=sha(json.dumps(config,sort_keys=True).encode())
  words=[int.from_bytes(bytes.fromhex(identity)[i:i+4],'little') for i in range(0,32,4)]
  (d/'diag_config.h').write_text('#define DIAG_MODE '+str(mode)+'\n#define IDENTITY_WORDS {'+','.join(hex(x) for x in words)+'}\n#define VECTOR_FAULT_ENTRIES '+','.join('nullptr' if i in (7,8,9,10,13) else 'Diag_Fault' for i in range(2,166))+'\n')
  flags=['-mcpu=cortex-m7','-mthumb','-mfloat-abi=soft','-O2','-g0','-ffreestanding','-fno-builtin','-fno-exceptions','-fno-rtti','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-fno-tree-loop-distribute-patterns','-ffunction-sections','-fdata-sections','-fstack-usage','-nostdlib','-Wall','-Wextra','-Werror']
  inc=[sdk/'Drivers/CMSIS-Device/ST/STM32H7xx/Include',sdk/'Drivers/CMSIS_5/CMSIS/Core/Include',d]
  elf=d/'diagnostic.elf'
  cmd=[compiler,*flags,'-std=c++17',*[x for p in inc for x in ['-I',p]],HERE/'diagnostic.cpp',HERE/'entry.S','-T',HERE/'diagnostic.ld','-Wl,--gc-sections,--no-undefined,-Map='+str(d/'diagnostic.map'),'-o',elf]
  (d/'build.log').write_text(run(cmd));(d/'command.json').write_text(json.dumps(list(map(str,cmd)),indent=2)+'\n')
  tool=lambda n:compiler.parent/('arm-none-eabi-'+n)
  run([tool('objcopy'),'-O','binary',elf,d/'diagnostic.bin'])
  dis=run([tool('objdump'),'-d',elf]);sections=run([tool('objdump'),'-h',elf]);segments=run([tool('readelf'),'-lW',elf]);symbols=run([tool('nm'),'-n',elf])
  require(not run([tool('nm'),'-u',elf]).strip(),'Undefined helper')
  require('cpsie' not in dis and not re.search(r'\b(svc|blx)\s',dis),'Unexpected unmask/indirect call')
  entry=run([tool('objdump'),'-d','--disassemble=Diag_Entry',elf])
  require(entry.find('cpsid')<entry.find('str'),'IRQ mask must precede instrumentation')
  if mode:require(entry.find('str')<entry.find('msr\tMSP'),'Progress must precede MSP switch')
  else:require('msr' not in entry and not re.search(r'\b(push|pop|bl)\s',entry),'Entry-only probe must not use/change stack')
  fault=run([tool('objdump'),'-d','--disassemble=Diag_Fault',elf])
  require(not re.search(r'\b(push|pop|bl|blx)\s',fault),'Fault capture must be stackless')
  for line in segments.splitlines():
   if line.strip().startswith('LOAD '):
    f=line.split();address=int(f[2],16);size=int(f[5],16)
    require(any(lo<=address and address+size<=hi for lo,hi in [(0x24040000,0x24041000),(0x24041000,0x24041080),(0x24042000,0x24042400),(0x2001fc00,0x20020000)]),'LOAD outside approved RAM')
  blob=(d/'diagnostic.bin').read_bytes();sp=int.from_bytes(blob[:4],'little');pc=int.from_bytes(blob[4:8],'little')
  require(664<=len(blob)<=4096 and sp==0x2001ffc0 and pc&1 and 0x24040000<=pc<0x24040000+len(blob),'Bad image/vector')
  for filename,text in [('disassembly.txt',dis),('sections.txt',sections),('segments.txt',segments),('symbols.txt',symbols),('entry-audit.txt',entry),('fault-audit.txt',fault)]: (d/filename).write_text(text)
  images.append(dict(name=name,mode=mode,artifact_id=identity,configuration=config,bin_file=name+'/diagnostic.bin',bin_sha256=sha(blob),bin_bytes=len(blob),elf_sha256=sha(elf.read_bytes()),initial_msp=sp,main_msp=0x2001ffc0,expected_halt_stage=2 if mode==0 else 17 if mode==1 else None))
 manifest=dict(schema='parable.ram-startup.v2',version='v0.2',code_address=0x24040000,code_limit=4096,mailbox_address=0x24041000,mailbox_bytes=128,trace_address=0x24042000,trace_bytes=1024,images=images,flash_programming_permitted=False,hardware_tested=False)
 (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps(dict(manifest_sha256=sha((out/'manifest.json').read_bytes()),images=[dict(name=i['name'],bytes=i['bin_bytes']) for i in images],hardware_tested=False)))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--libdaisy',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();build(a.compiler.resolve(),a.libdaisy.resolve(),a.output.resolve())
