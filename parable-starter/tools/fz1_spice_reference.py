#!/usr/bin/env python3
"""SF015 original-FZ1-style independent built-in PNP reference, not native DSP."""
import argparse,bisect,csv,hashlib,json,math,subprocess
from pathlib import Path
from rat_spice_reference import run_spice
TEMP=.0255/(1.380649e-23/1.602176634e-19)-273.15
CARD='IS=20.66u BF=229.6 BR=14.66 NF=1.133 NR=1.140 VAF=19.68 VAR=88.28 IKF=.463 IKR=.2415 ISE=2.190u NE=1.796 NC=1.364 RB=1.885 RE=.3064 RC=.000001727 CJE=0 CJC=100p MJC=0'
def circuit(attack=.5,bias=470e3,supply=3,source=1000,load=1e6,volume=.5,leakage=1,signal='0'):
    rp=attack*50000;rg=2200+22000*rp/(22000+rp)
    feed=f'Rbias rail b2 {bias:.17g}' if math.isfinite(bias) else '* Rbias physically open'
    top=f'RVtop top out {(1-volume)*500000:.17g}' if volume<1 else 'Vtop top out 0'
    bottom=f'RVbottom out 0 {volume*500000:.17g}' if volume>0 else 'Vbottom out 0 0'
    return f'''SF015 Amplified Parts original FZ1 drawing; patent typo reconciled
Vrail rail 0 {-supply:.17g}
Vin input 0 {signal}
Rsource input src {source:.17g}
R1 src inc 100k
C1 inc b1 10n
R2 b1 0 1Meg
Q1 rail b1 e1 Ge
R3 e1 0 10k
C2 e1 b2 20u
{feed}
Rattack b2 0 {rg:.17g}
Q2 c2 b2 0 Ge
R6 c2 rail 1.5k
C3 c2 b3 20u
R7 b3 0 10k
Q3 c3 b3 0 Ge
R8 c3 rail 10k
C4 c3 top 3.3n
R10 top 0 56k
{top}
{bottom}
Rload out 0 {load:.17g}
.model Ge PNP({CARD} ISC={7.546e-6*leakage:.17g} TNOM={TEMP:.17g} EG=.67 XTI=3 TF=0 TR=0)
.save all @q1[ic] @q2[ic] @q3[ic] @q1[ib] @q2[ib] @q3[ib]
'''
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def table(path,head,rows):
    with path.open('w',newline='') as f:w=csv.writer(f,lineterminator='\n');w.writerow(head);w.writerows(rows)
def generate(exe,out,fixtures,dc_only=False):
    out.mkdir(parents=True,exist_ok=True);fixtures.mkdir(parents=True,exist_ok=True);(out/'spinit').write_text('set num_threads=1\n')
    rows=[];temps=[];candidates=[]
    for fault in [0,1]:
        for attack in [0,.5,1]:
            for supply in [2.7,3,3.3]:
                for leakage in [.5,1,1.5]:
                    data=run_spice(exe,out,f'dc-{fault}-{attack}-{supply}-{leakage}',circuit(attack,math.inf if fault else 470e3,supply,leakage=leakage),f'set temp={TEMP:.17g}\nset numdgt=16\nop','v(b1) v(e1) v(b2) v(c2) v(b3) v(c3) @q1[ic] @q2[ic] @q3[ic] @q1[ib] @q2[ib] @q3[ib]')[0]
                    rows.append([fault,attack,supply,leakage,*data[1:]])
        for temp in [5,TEMP,40]:
            data=run_spice(exe,out,f'temp-{fault}-{temp}',circuit(bias=math.inf if fault else 470e3),f'set temp={temp:.17g}\nset numdgt=16\nop','v(b1) v(e1) v(b2) v(c2) v(b3) v(c3) @q1[ic] @q2[ic] @q3[ic]')[0]
            temps.append([fault,temp,*data[1:]])
    for attack in [0,.5,1]:
        for bias in [470,470e3,1e6,2.2e6,math.inf]:
            data=run_spice(exe,out,f'candidate-{attack}-{bias}',circuit(attack,bias),f'set temp={TEMP:.17g}\nset numdgt=16\nop','v(b2) v(c2) @q2[ic] @q2[ib]')[0]
            candidates.append([attack,'open' if math.isinf(bias) else bias,*data[1:]])
    nodes=['b1','e1','b2','c2','b3','c3'];currents=['q1_ic_A','q2_ic_A','q3_ic_A','q1_ib_A','q2_ib_A','q3_ib_A']
    table(fixtures/'dc.csv',['fault','attack','supply_V','leakage_scale',*nodes,*currents],rows)
    table(fixtures/'temperature.csv',['fault','temp_C',*nodes,*currents[:3]],temps)
    table(fixtures/'candidates.csv',['attack','bias_feed_ohms','b2','c2','q2_ic_A','q2_ib_A'],candidates)
    (fixtures/'nominal.cir').write_text(circuit())
    if not dc_only:
        cases=[]
        for fault in [0,1]:
            for attack in [.1,.5,1]:cases.append((f'pluck-{fault}-{attack}',fault,attack,.2,1000,1e6,.5,'pluck',.85))
            for level in [.02,.1,.2]:cases.append((f'sine-{fault}-{level}',fault,.5,level,1000,1e6,.5,'sine',.12))
            cases.append((f'loaded-{fault}',fault,.5,.1,10000,100e3,.5,'sine',.12))
            for volume in [0,1]:cases.append((f'volume-{fault}-{volume}',fault,.5,.1,1000,1e6,volume,'sine',.12))
        index=[]
        for name,fault,attack,level,source,load,volume,kind,duration in cases:
            s=circuit(attack,math.inf if fault else 470e3,source=source,load=load,volume=volume)
            if kind=='pluck':s=s.replace('Vin input 0 0',f'Binput input 0 V={{time<.01?0:{level}*(1-exp(-(time-.01)*3000))*exp(-(time-.01)/.12)*sin(2*3.141592653589793*110*(time-.01))}}')
            else:s=s.replace('Vin input 0 0',f'Vin input 0 SIN(0 {level} 196)')
            data=run_spice(exe,out,name,s,f'set temp={TEMP:.17g}\nset numdgt=16\noption method=gear reltol=1e-6 abstol=1e-12 vntol=1e-8\ntran 4u {duration} 0 4u','v(input) v(b1) v(e1) v(b2) v(c2) v(b3) v(c3) v(out) @q1[ic] @q2[ic] @q3[ic]')
            times=[r[0] for r in data];sampled=[]
            # Native192k uniform physical sample times; compact fixture every48 samples.
            for sample in range(0,round(duration*192000),48):
                t=sample/192000;hi=min(bisect.bisect_left(times,t),len(times)-1);lo=max(0,hi-1);f=(t-times[lo])/(times[hi]-times[lo]) if hi!=lo else 0
                sampled.append([sample,t,*[data[lo][i]+f*(data[hi][i]-data[lo][i]) for i in range(1,len(data[0]))]])
            table(fixtures/(name+'.csv'),['sample_192k','seconds','input_V',*nodes,'out_V',*currents[:3]],sampled)
            index.append(dict(name=name,fault=fault,attack=attack,input_V=level,source_Ohm=source,load_Ohm=load,volume=volume,kind=kind,duration_s=duration))
            (out/(name+'.txt')).unlink() # raw adaptive data reproducible; keep netlist+log.
            print('reference',name,flush=True)
        (fixtures/'transients.json').write_text(json.dumps(index,indent=2)+'\n')
    manifest=dict(simulator=subprocess.check_output([exe,'--version'],text=True).strip(),reference='Built-in PNP Gummel-Poon, measured AC128 template; not an empirical 2N270 or complete physical pedal',temperature_C=TEMP,netlists={p.name:digest(p) for p in sorted(out.glob('*.cir'))},fixtures={p.name:digest(p) for p in sorted(fixtures.glob('*.csv'))},native_equations_imported=False)
    (fixtures/'spice-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ngspice',required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True);p.add_argument('--dc-only',action='store_true');a=p.parse_args();generate(a.ngspice,a.output,a.fixtures,a.dc_only)
