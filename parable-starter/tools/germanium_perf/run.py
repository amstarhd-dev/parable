#!/usr/bin/env python3
"""Run UNINSTRUMENTED wall-time trials, or separate call counts; never ARM cycles."""
import argparse
import hashlib
import json
import os
import platform
import statistics
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
PROJECT=ROOT/'parable-starter'

def run(args,**kwargs):
    return subprocess.check_output(list(map(str,args)),text=True,**kwargs)

def summarize(data):
    groups={}
    for r in data['samples']:
        groups.setdefault((r['kind'],r['case'],r['implementation']),[]).append(r)
    out=[]
    for (kind,case,implementation),rows in groups.items():
        values=[r['ns_per_sample'] for r in rows]
        counts={field:sorted({r[field] for r in rows}) for field in
                ('solver_evaluations','failed','contained','max_evaluations','expm1_calls','sqrt_library_calls','approximation_calls')}
        out.append(dict(kind=kind,case=case,implementation=implementation,
                        best_ns=min(values),median_ns=statistics.median(values),worst_ns=max(values),counts=counts,
                        checksum_range=[min(r['checksum'] for r in rows),max(r['checksum'] for r in rows)]))
    return out

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--trials',type=int,default=7)
    p.add_argument('--repeats',type=int,default=32)
    p.add_argument('--cpu',type=int)
    a=p.parse_args()
    if hasattr(os,'sched_getaffinity'):
        cpu=a.cpu if a.cpu is not None else min(os.sched_getaffinity(0))
        os.sched_setaffinity(0,{cpu})
    binary=a.build.resolve()/'tools/germanium_perf/parable_germanium_bench'
    a.output.mkdir(parents=True,exist_ok=True)
    data=json.loads(run([binary,a.trials,a.repeats]))
    commands=json.loads((a.build/'compile_commands.json').read_text())
    metadata=dict(schema=1,source_commit=run(['git','rev-parse','HEAD'],cwd=ROOT).strip(),
                  dirty=bool(run(['git','status','--porcelain'],cwd=ROOT)),platform=platform.platform(),
                  cpu_model=next((x.split(':',1)[1].strip() for x in Path('/proc/cpuinfo').read_text().splitlines() if x.startswith('model name')),'unknown'),
                  affinity=sorted(os.sched_getaffinity(0)),compiler=run(['c++','--version']).splitlines()[0],
                  commands=[r for r in commands if 'germanium_perf' in r.get('command','')],
                  binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                  source_sha256={str(f.relative_to(ROOT)):hashlib.sha256(f.read_bytes()).hexdigest()
                                 for f in sorted([*PROJECT.glob('src/*germanium*'),*Path(__file__).parent.glob('*.cpp'),*Path(__file__).parent.glob('*.h')])},
                  measurement='instrumented call counts, NOT speed evidence' if data['instrumented'] else 'host VM wall time, NOT ARM cycles or guaranteed real-time feasibility')
    for name,obj in [('trials',data),('summary',summarize(data)),('environment',metadata)]:
        (a.output/(name+'.json')).write_text(json.dumps(obj,indent=2)+'\n')
    print(metadata['measurement'])
    print(f"{len(data['samples'])} observations; output {a.output}")
if __name__=='__main__':
    main()
