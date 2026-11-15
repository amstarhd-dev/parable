#!/usr/bin/env python3
"""Additional independent SPICE MNA reference of published GP equations.

Explicit exponential junction sources distinguish the full published law from
ngspice's built-in reverse-bias cubic continuation. No native code imported.
"""
import argparse,bisect,csv,hashlib,json,math,subprocess
from pathlib import Path
from fz1_spice_reference import circuit,TEMP,run_spice,table,digest

def network(**kw):
    s=circuit(**kw)
    for q,c,b,e in [(1,'rail','b1','e1'),(2,'c2','b2','0'),(3,'c3','b3','0')]:
        s=s.replace(f'Q{q} {c} {b} {e} Ge',f'Vqc{q} {c} cq{q} 0\nVqb{q} {b} bq{q} 0\nXq{q} cq{q} bq{q} {e} GP le={kw.get("leakage",1)}')
    s=s[:s.index('.model Ge')]
    # Paper2017 Eq1/4-7. SPICE solves all internal nodes and differential histories.
    # Measured RC1.727micro-ohm is shorted ONLY in this MNA reference: at1mA
    # its voltage <2nV, far below2uV gate; native and builtin keep finite RC.
    # Zero-volt terminal probes measure real terminal currents, not post-fit offsets.
    return s+'''.subckt GP c b e le=1
Rb b bi 1.885
Re e ei .3064
Vrc c ci 0
.func fwd(x) {20.66u*(exp(x/(1.133*.0255))-1)}
.func rev(x) {20.66u*(exp(x/(1.140*.0255))-1)}
Btransport ci ei I={-2*(1-v(ei,bi)/88.28-v(ci,bi)/19.68)*(fwd(v(ei,bi))-rev(v(ci,bi)))/(1+sqrt(1+4*(fwd(v(ei,bi))/.463+rev(v(ci,bi))/.2415)))}
Bbe bi ei I={-fwd(v(ei,bi))/229.6-2.190u*(exp(v(ei,bi)/(1.796*.0255))-1)}
Bbc bi ci I={-rev(v(ci,bi))/14.66-le*7.546u*(exp(v(ci,bi)/(1.364*.0255))-1)}
Cbc bi ci 100p
.ends GP
.save all
'''

def generate(exe,out,fixtures,dc_only=False):
    out.mkdir(parents=True,exist_ok=True);fixtures.mkdir(parents=True,exist_ok=True);(out/'spinit').write_text('set num_threads=1\n');rows=[]
    for fault in [0,1]:
        for attack in [0,.5,1]:
            for supply in [2.7,3,3.3]:
                for leakage in [.5,1,1.5]:
                    data=run_spice(exe,out,f'dc-{fault}-{attack}-{supply}-{leakage}',network(attack=attack,bias=math.inf if fault else 470e3,supply=supply,leakage=leakage),f'set numdgt=16\nop','v(b1) v(e1) v(b2) v(c2) v(b3) v(c3) i(Vqc1) i(Vqc2) i(Vqc3) i(Vqb1) i(Vqb2) i(Vqb3)')[0]
                    rows.append([fault,attack,supply,leakage,*data[1:]])
    table(fixtures/'equations-dc.csv',['fault','attack','supply_V','leakage_scale','b1','e1','b2','c2','b3','c3','q1_ic_A','q2_ic_A','q3_ic_A','q1_ib_A','q2_ib_A','q3_ib_A'],rows)
    if not dc_only:
        index=json.loads((fixtures/'transients.json').read_text())
        for case in index:
            name=case['name'];s=network(attack=case['attack'],bias=math.inf if case['fault'] else 470e3,source=case['source_Ohm'],load=case['load_Ohm'],volume=case['volume'])
            level=case['input_V'];duration=case['duration_s']
            if case['kind']=='pluck':s=s.replace('Vin input 0 0',f'Binput input 0 V={{time<.01?0:{level}*(1-exp(-(time-.01)*3000))*exp(-(time-.01)/.12)*sin(2*3.141592653589793*110*(time-.01))}}')
            else:s=s.replace('Vin input 0 0',f'Vin input 0 SIN(0 {level} 196)')
            data=run_spice(exe,out,name,s,f'set numdgt=16\noption method=gear reltol=1e-7 abstol=1e-13 vntol=1e-9\ntran 4u {duration} 0 4u','v(input) v(b1) v(e1) v(b2) v(c2) v(b3) v(c3) v(out) i(Vqc1) i(Vqc2) i(Vqc3)')
            times=[r[0] for r in data];sampled=[]
            if not times[0]<=0 or times[-1]<duration-1e-10:raise RuntimeError('Incomplete equation transient: '+name)
            for sample in sorted(set(range(0,round(duration*192000),48)) | set(range(6000,min(8049,round(duration*192000))))):
                t=sample/192000;hi=min(bisect.bisect_left(times,t),len(times)-1);lo=max(0,hi-1);f=(t-times[lo])/(times[hi]-times[lo]) if hi!=lo else 0
                sampled.append([sample,t,*[data[lo][i]+f*(data[hi][i]-data[lo][i]) for i in range(1,len(data[0]))]])
            table(fixtures/('equations-'+name+'.csv'),['sample_192k','seconds','input_V','b1','e1','b2','c2','b3','c3','out_V','q1_ic_A','q2_ic_A','q3_ic_A'],sampled)
            (out/(name+'.txt')).unlink();print('equations',name,flush=True)
    (fixtures/'equation-manifest.json').write_text(json.dumps(dict(simulator=subprocess.check_output([exe,'--version'],text=True).strip(),reference='Independent SPICE MNA, explicit full exponential GP from published2017 equations, not builtin compact-model continuation; constant caps/RB/RE, RC1.727micro-ohm replaced by exact short to avoid SPICE ill-conditioning; max dropped voltage <2nV at1mA',netlists={p.name:digest(p) for p in sorted(out.glob('*.cir'))},fixtures={p.name:digest(p) for p in sorted(fixtures.glob('equations-*.csv'))},source_sha256=digest(Path(__file__))),indent=2)+'\n')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ngspice',required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True);p.add_argument('--dc-only',action='store_true');a=p.parse_args();generate(a.ngspice,a.output,a.fixtures,a.dc_only)
