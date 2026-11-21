#!/usr/bin/env python3
"""Build and audit the freestanding RAM instrument; never accesses hardware."""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
def sha(data):return hashlib.sha256(data).hexdigest()
def run(args):return subprocess.check_output(list(map(str,args)),stderr=subprocess.STDOUT).decode()
def require(ok,message):
 if not ok:raise RuntimeError(message)
def build(compiler,sdk,out):
 pins=json.loads((ROOT/'firmware/dependencies.json').read_text())
 require(run([compiler,'-dumpfullversion']).strip()==pins['toolchain']['version'],'Wrong compiler version')
 require('xpack' in run([compiler,'--version']).lower(),'Wrong compiler distribution')
 require(run(['git','-C',sdk,'rev-parse','HEAD']).strip()==pins['libdaisy']['commit'],'Wrong SDK commit')
 require(not run(['git','-C',sdk,'status','--porcelain','--untracked-files=no']).strip(),'Modified SDK')
 for path,commit in pins['libdaisy']['submodules'].items():require(run(['git','-C',sdk/path,'rev-parse','HEAD']).strip()==commit,'Wrong submodule '+path)
 out.mkdir(parents=True,exist_ok=True)
 sources={p.name:sha(p.read_bytes()) for p in [HERE/'bootstrap.cpp',HERE/'bootstrap.ld',HERE/'protocol.h',HERE/'build.py']}
 config=dict(schema='parable.ram-identity-build.v1',sources=sources,pins=pins,compiler_sha256=sha(compiler.read_bytes()))
 identity=sha(json.dumps(config,sort_keys=True).encode())
 words=[int.from_bytes(bytes.fromhex(identity)[i:i+4],'little') for i in range(0,32,4)]
 (out/'identity_config.h').write_text('#define IDENTITY_WORDS {'+','.join(hex(x) for x in words)+'}\n#define VECTOR_FAULT_ENTRIES '+','.join('nullptr' if i in (7,8,9,10,13) else 'Ram_Fault' for i in range(2,166))+'\n')
 includes=[sdk/'Drivers/CMSIS-Device/ST/STM32H7xx/Include',sdk/'Drivers/CMSIS_5/CMSIS/Core/Include',out]
 elf=out/'ram-identity.elf'
 cmd=[compiler,'-mcpu=cortex-m7','-mthumb','-mfloat-abi=soft','-std=c++17','-O2','-g0','-ffreestanding','-fno-builtin','-fno-exceptions','-fno-rtti','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-fno-tree-loop-distribute-patterns','-fstack-usage','-nostdlib','-Wall','-Wextra','-Werror',*[v for i in includes for v in ['-I',i]],HERE/'bootstrap.cpp','-T',HERE/'bootstrap.ld','-Wl,--no-undefined,-Map='+str(out/'ram-identity.map'),'-o',elf]
 log=run(cmd);(out/'build.log').write_text(log);(out/'command.json').write_text(json.dumps(list(map(str,cmd)),indent=2)+'\n')
 tool=lambda n:compiler.parent/('arm-none-eabi-'+n)
 run([tool('objcopy'),'-O','binary',elf,out/'ram-identity.bin'])
 sections=run([tool('objdump'),'-h',elf]);disassembly=run([tool('objdump'),'-d',elf]);symbols=run([tool('nm'),'-n',elf])
 require(not run([tool('nm'),'-u',elf]).strip(),'Undefined runtime helper')
 require('cpsie' not in disassembly and not re.search(r'\b(svc|blx)\s',disassembly),'Unexpected interrupt enable or indirect call')
 for line in disassembly.splitlines():
  if re.search(r'\bbl\s',line):require('<Ram_Fault>' in line,'Unexpected runtime call')
 require('24040000' in sections and '24041000' in sections and '2001fc00' in sections,'Wrong section addresses')
 require('24040000' in symbols and 'Ram_Entry' in symbols and 'ram_identity_main' in symbols,'Missing entry symbols')
 for suffix,text in [('sections.txt',sections),('disassembly.txt',disassembly),('symbols.txt',symbols)]: (out/suffix).write_text(text)
 blob=(out/'ram-identity.bin').read_bytes();require(664<=len(blob)<=4096,'Image exceeds dedicated code region')
 sp=int.from_bytes(blob[:4],'little');entry=int.from_bytes(blob[4:8],'little')
 require(sp==0x20020000 and entry&1 and 0x24040000<=entry<0x24040000+len(blob),'Wrong initial vectors')
 require((0x52002000).to_bytes(4,'little') not in blob,'Unexpected FLASH controller literal') # STM32H750 FLASH controller base
 headers=run([tool('readelf'),'-lW',elf]);(out/'segments.txt').write_text(headers)
 for line in headers.splitlines():
  if line.strip().startswith('LOAD '):
   fields=line.split();address=int(fields[2],16);size=int(fields[5],16)
   require(any(lo<=address and address+size<=hi for lo,hi in [(0x24040000,0x24041080),(0x2001fc00,0x20020000)]),'ELF LOAD outside approved RAM')
 audit=dict(schema='parable.ram-startup-audit.v1',no_undefined_helpers=True,no_interrupt_enable_or_unexpected_calls=True,all_load_segments_in_documented_ram=True,no_c_runtime_data_or_bss=True,physical_tested=False,interrupt_policy='PRIMASK stays set until SYSRESETREQ; no USB app startup',peripheral_write_review=['SysTick/NVIC/SCB cache and reset','RCC USB reset and GPIOG clock','GPIOG pin3 only','128-byte AXI RAM mailbox; 1KiB DTCM stack'])
 (out/'audit.json').write_text(json.dumps(audit,indent=2)+'\n')
 manifest=dict(schema='parable.ram-identity.v1',version='v0.1',artifact_id=identity,configuration=config,bin_file='ram-identity.bin',bin_sha256=sha(blob),bin_bytes=len(blob),elf_sha256=sha(elf.read_bytes()),code_address=0x24040000,code_limit=4096,mailbox_address=0x24041000,mailbox_bytes=128,stack_bottom=0x2001fc00,stack_top=0x20020000,flash_programming_supported=False,hardware_tested=False)
 (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 print(json.dumps(dict(image_bytes=len(blob),manifest_sha256=sha((out/'manifest.json').read_bytes()),artifact_id=identity,hardware_tested=False)))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--libdaisy',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();build(a.compiler.resolve(),a.libdaisy.resolve(),a.output.resolve())
