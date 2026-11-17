#!/usr/bin/env python3
"""Compile complete unmodified research-core process paths; report blockers honestly."""
import argparse,hashlib,json,re,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--cache',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
compiler=a.compiler.resolve();cache=a.cache.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
pin=json.loads((ROOT/'firmware/dependencies.json').read_text())['toolchain'];assert subprocess.check_output([str(compiler),'-dumpfullversion'],text=True).strip()==pin['version']
flags=['-std=c++17','-O2','-fno-exceptions','-fno-rtti','-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-fstack-usage','-I'+str(cache),'-I'+str(cache/'include')]
rows=[]
for name,value in [('cloud',1),('signalsmith',0)]:
 obj=out/(name+'.o');command=[str(compiler),*flags,'-DENGINE_CLOUD='+str(value)]
 if value:command+=['-DBUFFER_SIZE=64','-DMAX_STR_SIZE=32','-include',str(ROOT/'tools/chamber_cloud_portability.h')]
 command+=['-c',str(ROOT/'tools/chamber_arm_probe.cpp'),'-o',str(obj)]
 run=subprocess.run(command,text=True,capture_output=True);(out/(name+'-compile.log')).write_text(run.stdout+run.stderr)
 if run.returncode:rows.append(dict(candidate=name,compiled=False,command=command,diagnostic=run.stderr));continue
 tool=lambda n:str(compiler.parent/('arm-none-eabi-'+n))
 undefined=subprocess.check_output([tool('nm'),'-u',str(obj)],text=True)
 assembly=subprocess.check_output([tool('objdump'),'-drC',str(obj)],text=True);(out/(name+'-process.disassembly.txt')).write_text(assembly)
 (out/(name+'-undefined.txt')).write_text(undefined)
 size=subprocess.check_output([tool('size'),str(obj)],text=True);text,data,bss=map(int,size.splitlines()[-1].split()[:3])
 subprocess.check_call([tool('objcopy'),'--dump-section','.rodata='+str(out/(name+'-layout.bin')),str(obj)])
 symbols=subprocess.check_output([tool('nm'),'-S',str(obj)],text=True)
 offsets={line.split()[-1]:int(line.split()[0],16) for line in symbols.splitlines() if line.split()[-1] in ('chamber_size','atomic_double_lockfree')}
 blob=(out/(name+'-layout.bin')).read_bytes();layout={s:struct.unpack_from('<I',blob,offset)[0] for s,offset in offsets.items()}
 names=sorted(line.split()[-1] for line in undefined.splitlines())
 rows.append(dict(candidate=name,compiled=True,object=dict(text=text,data=data,bss=bss),layout=layout,undefined=names,non_lock_free_64_bit_atomic_dependency='__atomic_load_8' in names,soft_double_dependencies=[n for n in names if n.startswith('__aeabi_d')],static_frames=(out/(name+'.su')).read_text().splitlines(),command=command,target_cycles='not measured',admitted=False))
report=dict(toolchain=pin,research_only=True,production_sources_modified=False,candidates=rows,limitations='Objects compile only selected process paths, not complete firmware, constructor code, dynamic memory placement, full transitive stack high-water or actual target execution. No pass/admission inference from a linkable object.')
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
