#!/usr/bin/env python3
"""Export untouched GP sources from Git and benchmark with the same present driver."""
import argparse
import hashlib
import json
import os
import platform
import subprocess
from pathlib import Path
from run import ROOT, summarize

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ref',default='440a99221af7cc3f3b7123db327bb161306d4856')
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--optimization',choices=['O2','O3'],default='O3')
    p.add_argument('--trials',type=int,default=7)
    p.add_argument('--repeats',type=int,default=128)
    a=p.parse_args()
    a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
    os.sched_setaffinity(0,{min(os.sched_getaffinity(0))})
    inputs=['src/parable_germanium.cpp','src/germanium_device.h','include/parable_germanium.h']
    hashes={}
    for f in inputs:
        b=subprocess.check_output(['git','show',a.ref+':parable-starter/'+f],cwd=ROOT)
        target=a.output/'original'/f;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(b)
        hashes[f]=hashlib.sha256(b).hexdigest()
    driver=Path(__file__).resolve().parent
    flags=['-std=c++17','-'+a.optimization,'-DNDEBUG','-fno-fast-math','-fno-finite-math-only','-DPERF_BASELINE',
           '-I'+str(a.output/'original/include'),'-I'+str(a.output/'original/src'),'-I'+str(driver)]
    metadata=dict(source_ref=a.ref,source_sha256=hashes,driver_sha256={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in driver.glob('*') if f.suffix in ('.cpp','.h')},
                  compiler=subprocess.check_output(['c++','--version'],text=True).splitlines()[0],
                  flags=flags,affinity=sorted(os.sched_getaffinity(0)),platform=platform.platform(),
                  measurement='host VM only; same workload as initial baseline; present driver, source exported from unchanged main')
    for name,counting,trials,repeats in [('timing',False,a.trials,a.repeats),('counts',True,3,1)]:
        objects=[]
        for n,source in enumerate([a.output/'original/src/parable_germanium.cpp',driver/'perf_bridge.cpp',driver/'germanium_bench.cpp']):
            obj=a.output/(name+str(n)+'.o')
            extra=['-Dparable=parable_gp_libm'] if n<2 else (['-DPERF_COUNT'] if counting else [])
            subprocess.run(['c++',*flags,*extra,'-c',str(source),'-o',str(obj)],check=True)
            objects.append(str(obj))
        binary=a.output/name
        subprocess.run(['c++',*objects,*(['-Wl,--wrap=expm1','-Wl,--wrap=sqrt'] if counting else []),'-o',str(binary)],check=True)
        data=json.loads(subprocess.check_output([binary,str(trials),str(repeats)],text=True))
        (a.output/(name+'.json')).write_text(json.dumps(data,indent=2)+'\n')
        (a.output/(name+'-summary.json')).write_text(json.dumps(summarize(data),indent=2)+'\n')
    (a.output/'environment.json').write_text(json.dumps(metadata,indent=2)+'\n')
if __name__=='__main__':
    main()
