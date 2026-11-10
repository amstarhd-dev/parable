#!/usr/bin/env python3
"""Compare standalone numerical variants using the verified build.py toolchain."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

PROJECT=Path(__file__).resolve().parents[2]
PINS=json.loads((PROJECT/'firmware/dependencies.json').read_text())
FLAGS=['-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-std=gnu++17','-O2','-fno-exceptions','-fno-rtti','-fshort-enums','-fno-fast-math','-fno-finite-math-only','-fstack-usage']
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cache',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    archive=a.cache/'arm-toolchain.tar.gz'
    if hashlib.sha256(archive.read_bytes()).hexdigest()!=PINS['toolchain']['sha256']:
        raise RuntimeError('Toolchain archive hash differs; run firmware/build.py first')
    tool=a.cache/PINS['toolchain']['directory']/'bin'
    cc=tool/'arm-none-eabi-g++'
    if subprocess.check_output([cc,'-dumpfullversion'],text=True).strip()!=PINS['toolchain']['version']:
        raise RuntimeError('Compiler version differs')
    a.output.mkdir(parents=True,exist_ok=True)
    report=dict(flags=FLAGS,toolchain_sha256=PINS['toolchain']['sha256'],variants={},hardware_cycles='not measured',linked_into_firmware=False,
                source_sha256={str(f.relative_to(PROJECT)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted([*PROJECT.glob('src/*germanium*'),PROJECT/'include/parable_germanium.h'])})
    for mode,label in enumerate(('libm','hermite32','hermite64','polynomial8','polynomial12')):
        obj=a.output/(label+'.o')
        compile_args=[str(cc),*FLAGS,f'-DPARABLE_GERMANIUM_EVALUATOR={mode}','-I',str(PROJECT/'include'),'-c',str(PROJECT/'src/parable_germanium.cpp'),'-o',str(obj)]
        subprocess.run(compile_args,check=True)
        texts={}
        for name,args in {'size':['size','-A'], 'berkeley':['size'], 'elf':['readelf','-h','-A'], 'symbols':['nm','-C','-S'], 'undefined':['nm','-C','-u'], 'disassembly':['objdump','-drC']}.items():
            text=subprocess.check_output([tool/('arm-none-eabi-'+args[0]),*args[1:],obj],text=True)
            (a.output/(label+'-'+name+'.txt')).write_text(text);texts[name]=text
        if re.search(r'operator new|malloc|calloc|realloc|__cxa_throw|pthread|printf|fstream',texts['undefined']):
            raise RuntimeError('Unexpected callback dependency')
        sections={}
        for line in texts['size'].splitlines():
            fields=line.split()
            if len(fields)==3 and fields[0].startswith('.'):
                sections[fields[0]]=int(fields[1])
        dis=texts['disassembly']
        sizes=[int(v) for v in texts['berkeley'].splitlines()[-1].split()[:3]]
        report['variants'][label]=dict(command=compile_args,sections=sections,
             text_including_rodata=sizes[0],data=sizes[1],bss=sizes[2],
             compiler_stack_frames=[dict(function=row.rsplit('\t',2)[0],bytes=int(row.rsplit('\t',2)[1]),kind=row.rsplit('\t',2)[2]) for row in obj.with_suffix('.su').read_text().splitlines()],
             read_only_tables=sum(size for section,size in sections.items() if 'tables' in section),
             expm1_relocations=len(re.findall(r'R_ARM_THM_CALL\s+expm1\b',dis)),
             sqrt_relocations=len(re.findall(r'R_ARM_THM_CALL\s+sqrt\b',dis)),
             vsqrt=len(re.findall(r'\bvsqrt\.f64\b',dis)),vdiv=len(re.findall(r'\bvdiv\.f64\b',dis)),
             vmul=len(re.findall(r'\bvmul\.f64\b',dis)),vfma=len(re.findall(r'\bvfma\.f64\b',dis)),
             object_sha256=hashlib.sha256(obj.read_bytes()).hexdigest())
    (a.output/'comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':
    main()
