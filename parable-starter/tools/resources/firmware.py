#!/usr/bin/env python3
"""Produce source-identifiable experimental Seed3 benchmark images. NEVER flashes/merges."""
import argparse,gzip,json,shlex,subprocess
from pathlib import Path
from run import HERE,ROOT,Sources,validate,command,digest,require
from hardware import matrix

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--libdaisy',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--only',nargs='+',required=True);p.add_argument('--mode',type=int,choices=[0,1,2],default=2);p.add_argument('--capture-blocks',type=int,default=3000);p.add_argument('--execution',choices=['foreground_kernel','audio_callback_body'],default='foreground_kernel');p.add_argument('--layout',choices=['BOOT_NONE','BOOT_SRAM','BOOT_QSPI'],default='BOOT_NONE');p.add_argument('--usb-control',action='store_true',help='experimental reset/CDC control; BOOT_NONE only');p.add_argument('--fetch',action='store_true');a=p.parse_args()
 if a.usb_control and a.layout!='BOOT_NONE':p.error('Automated ROM handoff supports BOOT_NONE only')
 if not 1<=a.capture_blocks<=30000:p.error('capture-blocks1..30000')
 compiler=a.compiler.resolve();sdk=a.libdaisy.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 pins=json.loads((ROOT/'firmware/dependencies.json').read_text());require(command([compiler,'-dumpfullversion']).strip()==pins['toolchain']['version'],'Wrong compiler version');require('xpack' in command([compiler,'--version']).lower(),'Wrong compiler distribution');require(command(['git','-C',sdk,'rev-parse','HEAD']).strip()==pins['libdaisy']['commit'],'Wrong SDK commit');require(not command(['git','-C',sdk,'status','--porcelain','--untracked-files=no']).strip(),'Modified SDK')
 for path,sha in pins['libdaisy']['submodules'].items():require(command(['git','-C',sdk/path,'rev-parse','HEAD']).strip()==sha,'Wrong SDK submodule')
 archive=sdk/'build/libdaisy.a';require(archive.exists(),'Build exact pinned SDK archive first')
 catalog=validate(json.loads((HERE/'catalog.json').read_text()));models={m['id']:m for m in catalog['models']};profiles={m['id']:m for m in catalog['models']+catalog['chains']}
 if set(a.only)-set(profiles):p.error('Unknown profile')
 sources=Sources(out,a.fetch);artifacts=[]
 for identity in a.only:
  profile=profiles[identity];selected_models=[models[m] for m in profile.get('members',[identity])]
  row=dict(control_protocol='parable.control.v1' if a.usb_control else None,handoff='reset-systeminit-v1' if a.usb_control else None,flash_origin=0x08000000,flash_length=131072,profile=identity,execution=a.execution,stimulus='synthetic_mix',mode=a.mode,route=profile.get('route',0),capture_blocks=a.capture_blocks,status='pending',layout=a.layout)
  directory=None
  try:
   if any(m['id']=='cancer_cloud' for m in selected_models):raise RuntimeError('Cloud memory hold: do not produce misleading target image')
   if any(m['id']=='taurus_plate' for m in selected_models):raise RuntimeError('Plate vector heap needs separately validated target placement before image generation')
   if any(m['adapter']=='Utility' for m in selected_models):raise RuntimeError('Use standalone RAT/drive or chain image; utility firmware images are not selected yet')
   files=[];includes=[];definitions=['BENCH_ARM']+(['BENCH_USB_CONTROL'] if a.usb_control else []);forced=[];arena_samples=0;source_hashes={}
   for m in selected_models:
    cache,c,i=sources.extract(m);files+=c;includes+=i;definitions+=m['defines'];forced+=[cache/f for f in m.get('forced_includes',[])];source_hashes.update(sources.current_hashes)
    if m['adapter'] in ['Tape','Rooms','Tiled']:arena_samples+=192000 if m['adapter']=='Tape' else 24576
   selected=selected_models[0]['adapter'] if len(selected_models)==1 else 'Chain<'+','.join(m['adapter'] for m in selected_models)+'>'
   definitions+=['BENCH_SELECTED='+selected]
   if len(selected_models)>1:definitions+=['BENCH_ROUTE']
   large=any(m['adapter']=='Hall' for m in selected_models)
   if large:definitions+=['BENCH_MODEL_SDRAM']
   if a.execution=='audio_callback_body':definitions+=['BENCH_AUDIO_CALLBACK']
   source_set_sha=digest(json.dumps(source_hashes,sort_keys=True).encode())
   configuration=dict(usb_control=a.usb_control,profile=profile,mode=a.mode,execution=a.execution,layout=a.layout,capture_blocks=a.capture_blocks,source_sha256=source_hashes,harness_sha256={f.name:digest(f.read_bytes()) for f in [HERE/'firmware_bench.cpp',HERE/'adapters.h',HERE/'firmware.py',HERE/'run.py',ROOT/'firmware/runtime_measurement.h',HERE/'dfu_control.h',HERE/'dfu_handoff.cpp',HERE/'dfu_handoff.ld']},compiler_sha256=digest(compiler.read_bytes()),libdaisy_archive_sha256=digest(archive.read_bytes()),pins=pins,linker_sha256=digest((sdk/pins['target']['layouts'][a.layout]).read_bytes()),definitions=definitions,arena_samples=arena_samples)
   artifact_id=digest(json.dumps(configuration,sort_keys=True).encode());name=f'parres-v1-{identity}-m{a.mode}-{artifact_id[:12]}-{a.layout.lower()}'
   directory=out/name;directory.mkdir(parents=True,exist_ok=True)
   # Arrays are deliberately reserved at conservative supported capacities.
   # ELF symbol sizes below replace modelBytes with actual targetsizeof.
   model_sdram_bytes=0
   config=directory/'resource_config.h'
   config.write_text('\n'.join(['#pragma once',f'#define BENCH_PROFILE {json.dumps(identity)}',f'#define BENCH_ARTIFACT_ID {json.dumps(artifact_id)}',f'#define BENCH_SOURCE_SHA {json.dumps(source_set_sha)}',f'#define BENCH_EXECUTION {json.dumps(a.execution)}',f'#define BENCH_MODE {a.mode}',f'#define BENCH_ROUTE_VALUE {profile.get("route",0)}',f'#define BENCH_CAPTURE_BLOCKS {a.capture_blocks}',f'#define BENCH_ARENA_SAMPLES {arena_samples}',f'#define BENCH_MODEL_SDRAM_BYTES '+('sizeof(Selected)' if large else '0'),f'#define BENCH_RESERVED_SDRAM_BYTES ({arena_samples}*4+BENCH_MODEL_SDRAM_BYTES)'])+'\n')
   cppflags=['-g0','-fno-fast-math','-fno-finite-math-only','-fstack-usage',*['-D'+d for d in dict.fromkeys(definitions)],*[x for f in forced for x in ['-include',str(f)]]]
   includes+=[HERE,ROOT/'firmware',directory,sdk/'Middlewares/Patched/ST/STM32_USB_Device_Library/Class/CDC/Inc']
   handoff_flags=f'LDFLAGS = -Wl,--wrap=SystemInit -T{HERE}/dfu_handoff.ld\n' if a.usb_control else ''
   make=directory/'Makefile';make.write_text(f'TARGET = {name}\nLIBDAISY_DIR = {sdk}\nCPP_SOURCES = '+ ' '.join(map(str,[*dict.fromkeys(files),HERE/'firmware_bench.cpp',*([HERE/'dfu_handoff.cpp'] if a.usb_control else [])]))+'\nC_INCLUDES = '+' '.join('-I'+str(i) for i in dict.fromkeys(includes))+f'\nSYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core\nCPP_STANDARD = -std=gnu++17\nAPP_TYPE = {a.layout}\nOPT = -O2\n{handoff_flags}include $(LIBDAISY_DIR)/core/Makefile\nCPPFLAGS += '+' '.join(shlex.quote(x) for x in cppflags)+f'\nCFLAGS += -g0 -fstack-usage\n')
   build=directory/'build';r=subprocess.run(['make','-C',str(directory),'-j2','GCC_PATH='+str(compiler.parent),'BUILD_DIR='+str(build)],capture_output=True,text=True);(directory/'build.log').write_text(r.stdout+r.stderr)
   if r.returncode:raise RuntimeError('Link failed; see '+str(directory/'build.log'))
   elf=build/(name+'.elf');binary=build/(name+'.bin');tool=lambda n:compiler.parent/('arm-none-eabi-'+n)
   sections=command([tool('size'),'-A',elf]);symbols=command([tool('nm'),'-SC',elf]);(directory/'sections.txt').write_text(sections)
   for line in symbols.splitlines():
    fields=line.split()
    if len(fields)==4 and fields[3]=='modelBytes':model_sdram_bytes=int(fields[1],16) if large else 0
   parents=json.loads((ROOT/'docs/resources/artifacts.json').read_text())['artifacts'];parent=next((v for v in parents if v['profile']==identity and v['mode']==a.mode and v['execution']==a.execution),None)
   row['parent_artifact_id']=parent['artifact_id'] if parent else None
   row.update(status='linked',artifact_id=artifact_id,source_set_sha256=source_set_sha,bin_sha256=digest(binary.read_bytes()),elf_sha256=digest(elf.read_bytes()),bin_file=str(binary.relative_to(out)),configuration=configuration,reserved_sdram_bytes=arena_samples*4+model_sdram_bytes,model_sdram_bytes=model_sdram_bytes,sections=sections)
   (directory/'symbols.txt.gz').write_bytes(gzip.compress(symbols.encode(),mtime=0));(build/(name+'.elf.gz')).write_bytes(gzip.compress(elf.read_bytes(),mtime=0));elf.unlink()
   for f in build.iterdir():
    if f.suffix in ['.o','.lst','.d']:f.unlink()
    elif f.suffix=='.map':f.with_suffix('.map.gz').write_bytes(gzip.compress(f.read_bytes(),mtime=0));f.unlink()
  except (RuntimeError,subprocess.CalledProcessError,OSError) as e:
   row.update(status='failed',error=str(e))
   if directory:
    for generated in (directory/'build').glob('*'):
     if generated.suffix in ['.o','.lst','.d','.hex']:generated.unlink()
  artifacts.append(row);print(identity,row['status'],row.get('bin_file',row.get('error')),flush=True)
  manifest=dict(schema='parable.resource-artifacts.v1',hardware_run=False,artifacts=artifacts,limits='Original pinned sources; link/build evidence only. Foreground images never start audio; callback option is experimental, IRQ-starvation risk if budget exceeded. Official SDK layout used unchanged; bootloader-dependent links do not prove Seed3 boot compatibility.')
  (out/'artifacts.json').write_text(json.dumps(manifest,indent=2)+'\n');matrix(manifest,out/'hardware-matrix.csv')
 return int(any(x['status']!='linked' for x in artifacts))
if __name__=='__main__':raise SystemExit(main())
