#!/usr/bin/env python3
"""Independent Decimal impulse/projection reference and closed-form forced modes.
No native executable/output, discrete rotation or contact helper is consumed.
"""
import argparse
import cmath
import csv
from decimal import Decimal as D, getcontext
import hashlib
import json
import math
from pathlib import Path


def generate(out):
    out.mkdir(parents=True,exist_ok=True);getcontext().prec=60
    # Three orthogonal energy coordinates: third is an HF mode. Arbitrary
    # EXACT decimal initial states/shape; no fitting to the 192-mode DSP.
    d=[D('.3'),D('-.1'),D('.02')];c=[D('2'),D('-1'),D('3')]
    dd=sum(v*v for v in d);cc=sum(v*v for v in c);e=D('.55');gap=D('.01')
    with (out/'contact.csv').open('w',newline='') as f:
        w=csv.writer(f,lineterminator='\n')
        w.writerow(['case','x0','x1','x2','v0','v1','v2','after_x0','after_x1','after_x2','after_v0','after_v1','after_v2','projection_J','restitution_J','HF_kinetic_gain_J'])
        for name,xs,vs in [('outward',['.1','0','0'],['.1','0','0']),('inward',['.1','0','0'],['-.1','0','0']),('clear',['.01','0','0'],['.1','0','0']),('negative_clear',['-.1','0','0'],['-.1','0','0'])]:
            x=list(map(D,xs));v=list(map(D,vs));y=sum(a*b for a,b in zip(d,x));u=sum(a*b for a,b in zip(c,v));xx=x[:];vv=v[:]
            if y>gap:
                # Orthogonal closest-point minimizer of ||xx-x|| subject d.xx=gap.
                xx=[a-b*(y-gap)/dd for a,b in zip(x,d)]
                if u>0:vv=[a-(1+e)*u*b/cc for a,b in zip(v,c)]
            potential=(sum(a*a for a in x)-sum(a*a for a in xx))/2
            kinetic=(sum(a*a for a in v)-sum(a*a for a in vv))/2
            w.writerow([name,*x,*v,*xx,*vv,potential,kinetic,(vv[2]**2-v[2]**2)/2])
    with (out/'linear.csv').open('w',newline='') as f:
        w=csv.writer(f,lineterminator='\n');w.writerow(['end_seconds','secondary_pickup_m_s'])
        for tick in range(1,101):
            t=tick/1000;duration=min(t,.005);value=0;stretch=1.;stiffness=.04
            # Exact convolution of dz/dt=(-sigma+i*omega)z + F*b,
            # z=v+i*x, F=.002*.05*sin(pi*t/.005), truncated after5ms.
            a=math.pi/.005
            for n in range(1,97):
                hz=31*n*stretch;lam=complex(-(3+8*(hz/8000)**2),2*math.pi*hz)
                def integral(k):return (cmath.exp((k-lam)*duration)-1)/(k-lam)
                convolution=cmath.exp(lam*t)*(integral(1j*a)-integral(-1j*a))/(2j)
                b=.5*math.cos(n*math.pi*.03)/math.sqrt(.0003)
                pickup=.5*math.cos(n*math.pi*.88)/math.sqrt(.0003)
                value+=pickup*(.002*.05*b*convolution).real
                stretch+=stiffness;stiffness*=.98
            w.writerow([format(t,'.3f'),format(value,'.12f')])
    manifest={'method':'60-digit Decimal orthogonal energy projection/restitution; continuous complex exponential convolution for untouched second path','linear_decimal_places':12,'native_inputs':'none','independence':'No native helper, split-step, rendered output or fitted constants. Equivalent assumptions shared, not measured tank/SPICE truth.','files_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.glob('*.csv'))}}
    (out/'reference-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);generate(p.parse_args().output)
