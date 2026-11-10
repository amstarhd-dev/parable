#!/usr/bin/env python3
"""Additional independent rail/source/gain corners; original SF003 fixtures unchanged."""
import argparse
import bisect
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from germanium_spice_reference import circuit, reference, write_csv

CASES=[
 dict(name='healthy_lowrail',rail=6,source=100,fuzz=1,level=.05,leak=7.546e-6,feedback=100000,beta1=229.6,beta2=229.6),
 dict(name='leaky_highsource_lowrail',rail=6,source=100000,fuzz=.8,level=.05,leak=75.46e-6,feedback=100000,beta1=229.6,beta2=229.6),
 dict(name='feedback_highsource',rail=9,source=100000,fuzz=.8,level=.05,leak=7.546e-6,feedback=10000,beta1=229.6,beta2=229.6),
 dict(name='mixed_gain_lowrail',rail=6,source=1000,fuzz=.8,level=.005,leak=100e-6,feedback=100000,beta1=40,beta2=300)]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ngspice',required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--fixtures',type=Path,required=True)
    a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=True);a.fixtures.mkdir(parents=True,exist_ok=True)
    (a.output/'spinit').write_text('* SF003 independent PNP reference\nset num_threads=1\n')
    version=subprocess.check_output([a.ngspice,'--version'],text=True).strip()
    if 'ngspice-44.2' not in version:raise ValueError('Use the existing SHA-pinned ngspice44.2 package')
    bias=[];transient=[];vectors='v(b1) v(c1) v(e2) v(c2) v(tap) v(jack) i(Vinput)'
    for index,c in enumerate(CASES):
        network=circuit(c['fuzz'],c['source'],c['level'],c['leak'],c['feedback'])
        network=network.replace('Vbattery supply 0 -9',f"Vbattery supply 0 -{c['rail']}")
        network=network.replace('Xq1 c1 b1 0 measured ',f"Xq1 c1 b1 0 measured beta={c['beta1']:.17g} ")
        network=network.replace('Xq2 c2 c1 e2 measured\n',f"Xq2 c2 c1 e2 measured beta={c['beta2']:.17g}\n")
        dc=reference(a.ngspice,a.output,'bias-'+c['name'],network,'op',vectors)[0]
        bias.append([index,*dc[1:7]])
        rows=reference(a.ngspice,a.output,'transient-'+c['name'],network,'tran 0.25u 0.012 0 0.25u',vectors)
        times=[r[0] for r in rows]
        for sample in range(1,1153,6):
            time=sample/96000;hi=bisect.bisect_left(times,time);lo,hi=max(0,hi-1),min(hi,len(rows)-1)
            if not times[lo]<=time<=times[hi]:raise ValueError('Missing transient span')
            mix=(time-times[lo])/(times[hi]-times[lo]) if hi!=lo else 0
            values=[rows[lo][i]+mix*(rows[hi][i]-rows[lo][i]) for i in range(1,8)]
            values[-1]=-values[-1]
            transient.append([index,sample,*values])
    write_csv(a.fixtures/'bias.csv',['case','b1','c1','e2','c2','tap','jack'],bias)
    write_csv(a.fixtures/'transient.csv',['case','sample96k','b1','c1','e2','c2','tap','jack','input_amps'],transient)
    (a.fixtures/'cases.json').write_text(json.dumps(CASES,indent=2)+'\n')
    netlists=a.fixtures/'netlists';netlists.mkdir(exist_ok=True)
    for f in a.output.glob('*.cir'):shutil.copyfile(f,netlists/f.name)
    manifest=dict(simulator=version,scope='Independent ngspice solution of SAME reduced GP circuit, not physical calibration',
                  maximum_step_seconds=.25e-6,original_equations='tools/germanium_spice_reference.py',
                  cases=CASES,netlists_sha256={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(netlists.glob('*.cir'))},
                  fixtures_sha256={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(a.fixtures.glob('*.csv'))})
    (a.fixtures/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
if __name__=='__main__':main()
