#!/usr/bin/env python3
"""Offline independent SPICE TS808 reference, never consumes native stamps/output."""
import argparse,bisect,csv,hashlib,json,math,shutil,subprocess
from pathlib import Path
from rat_spice_reference import run_spice
NODES=['b1','e1','plus','minus','o1','tp','tm','o2','leveltop','b3','e3','jack']
CAPS=['v(in,ci)','v(e1,plus)','v(minus,cr)','v(minus,o1)','v(tp)','v(tw,cc)','v(o2,cp)','v(lw,b3)','v(ob,jack)']
def circuit(fault=False,drive=.5,tone=.5,level=.5,amplitude=.2,frequency=1000,triangle=False):
 def r(name,a,b,v):return f'R{name} {a} {b} {v:.17g}\n' if v else f'V{name} {a} {b} 0\n'
 c=f'''ElectroSmash TS808 true-bypass drawing, effect-on only, explicit nominal devices
Vcc rail 0 9
Vref ref 0 4.5
Vsrc source 0 SIN(0 {amplitude:.17g} {frequency:.17g}) AC 1
Rsource source in 1k
C1 in ci 20n
R1 ci b1 1k
R2 ref b1 510k
Q1 rail b1 e1 silicon
R3 e1 0 10k
C2 e1 plus 1u
R5 plus ref 10k
R4 cr 0 4.7k
C3 minus cr 47n
R6 minus o1 {51000+500000*(20**drive-1)/19:.17g}
C4 minus o1 51p
D1 minus o1 diode
'''
 if not fault:c+='D2 o1 minus diode\n'
 c+='''R8 o1 tp 1k
C5 tp 0 220n
R9 tp ref 10k
R10 cc 0 220
C6 tw cc 220n
R11 tm o2 1k
'''+r('tone1','tp','tw',20000*tone)+r('tone2','tw','tm',20000*(1-tone))+'''C7 o2 cp 1u
R12 cp leveltop 1k
'''+r('level1','leveltop','lw',100000*(1-level))+r('level2','lw','0',100000*level)+'''C8 lw b3 100n
R13 ref b3 510k
Q3 rail b3 e3 silicon
R14 e3 0 10k
RB e3 ob 100
C9 ob jack 10u
RC jack 0 10k
Rload jack 0 1Meg
.model silicon NPN(IS=1e-14 BF=350 BR=4 VAF=0 VAR=0 CJE=0 CJC=0 TF=0 TR=0 TNOM=26.85)
.model diode D(IS=2.52n N=1.752 RS=0 CJO=0 TT=0 TNOM=26.85)
'''
 for k,p,m,o in [('1','plus','minus','o1'),('2','tp','tm','o2')]:
  c+=f'''Rdiff{k} {p} {m} 5Meg
Cpole{k} u{k} 0 1n
Bamp{k} 0 u{k} I={{1e-9*(max(-1e6,min(1e6,188.4955592153876*(1e5*v({p},{m})-v(u{k}))))-1e9*(v(u{k})-max(-3,min(3,v(u{k})))) )}}
Eamp{k} drive{k} ref u{k} 0 1
Rout{k} drive{k} {o} 75
'''
 if triangle:c=c.replace(f'Vsrc source 0 SIN(0 {amplitude:.17g} {frequency:.17g}) AC 1',f'Bsrc source 0 V={{{amplitude:.17g}*(2/PI)*asin(sin(2*PI*{frequency:.17g}*time))}}')
 c+='\n.save all @d1[id]'+(' @d2[id]' if not fault else '')+'\n'
 return c

def write(path,header,rows):
    with path.open('w',newline='') as f:
        w=csv.writer(f,lineterminator='\n');w.writerow(header);w.writerows(rows)

def generate(exe,out,fixtures):
    out.mkdir(parents=True,exist_ok=True);fixtures.mkdir(parents=True,exist_ok=True)
    (out/'spinit').write_text('* explicit isolated SF010 startup\nset num_threads=1\n')
    version=subprocess.check_output([exe,'--version'],text=True).strip()
    def reference(name,c,analysis,vector,end=None):
        rows=run_spice(exe,out,name,c,analysis,vector)
        log=(out/(name+'.log')).read_text().lower()
        if 'aborted' in log or 'fatal' in log or not rows or not all(math.isfinite(v) for r in rows for v in r):raise RuntimeError('Invalid reference:'+name)
        if end is not None and (rows[0][0]>1e-9 or rows[-1][0]<end-1e-10):raise RuntimeError('Truncated reference:'+name)
        return rows
    bias=[]
    for fault in [False,True]:
        for setting in [0.,.5,1.]:
            rows=reference(f'bias-{int(fault)}-{setting}',circuit(fault,setting,setting,setting),'op',' '.join('v('+n+')' for n in NODES+['u1','u2']))
            bias.append([int(fault),setting,*rows[0][1:]])
    write(fixtures/'bias.csv',['fault','control',*NODES,'u1','u2'],bias)
    devices=[]
    for collector in [.1,3.,9.]:
        c=f'''Independent builtin nominal NPN
Vb b 0 0
Vc c 0 {collector}
Q c b 0 silicon
.model silicon NPN(IS=1e-14 BF=350 BR=4 VAF=0 VAR=0 CJE=0 CJC=0 TF=0 TR=0 TNOM=26.85)
'''
        rows=reference('device-'+str(collector),c,'dc Vb -.1 .7 .1','v(b) v(c) i(Vb) i(Vc)')
        devices.extend([[r[1],r[1]-r[2],-r[3],-r[4]] for r in rows])
    write(fixtures/'device.csv',['vbe_V','vbc_V','base_A','collector_A'],devices)
    c='''Independent builtin feedback diode
Vd a 0 0
D a 0 silicon
.model silicon D(IS=2.52n N=1.752 RS=0 CJO=0 TT=0 TNOM=26.85)
'''
    rows=reference('diode',c,'dc Vd -.8 .8 .05','v(a) i(Vd)')
    write(fixtures/'diode.csv',['anode_minus_cathode_V','current_A'],[[r[1],-r[2]] for r in rows])
    ac=[]
    controls=[('nominal',.5,.5,.5),('drive0',0,.5,.5),('drive1',1,.5,.5),('tone0',.5,0,.5),('tone1',.5,1,.5),('level0',.5,.5,0),('level1',.5,.5,1)]
    for case,d,t,l in controls:
        for frequency in [100,125,250,500,1000,2000,4000,8000,10000]:
            rows=reference(f'ac-{case}-{frequency}',circuit(False,d,t,l),f'ac lin 1 {frequency} {frequency}','real(v(e1)) imag(v(e1)) real(v(o1)) imag(v(o1)) real(v(o2)) imag(v(o2)) real(v(jack)) imag(v(jack))')
            ac.append([case,d,t,l,frequency,*rows[0][1:]])
    write(fixtures/'ac.csv',['case','drive','tone','level','frequency_hz','input_re','input_im','clip_re','clip_im','tone_re','tone_im','jack_re','jack_im'],ac)
    transient=[];fractional=[];metrics=[];refinement=[]
    cases=[('low',.5,.005,1000,False),('medium',.5,.05,1000,False),('hard',.5,.2,1000,False),('drive0',0,.2,1000,False),('drive1',1,.2,1000,False),('triangle',.5,.3,1000,True),('slew',1,.8,10000,False)]
    for fault in [False,True]:
        for case,d,amplitude,frequency,triangle in cases:
            for maxstep in ([.5e-6,.125e-6] if fault and case=='hard' else [.5e-6]):
                name=f'transient-{int(fault)}-{case}-{maxstep}'
                analysis=f'option method=gear maxord=2 reltol=1e-5 abstol=1e-10 vntol=1e-7\ntran 1u .12 0 {maxstep:.17g}'
                vector=' '.join(['v('+n+')' for n in NODES+['u1','u2']]+CAPS+['@d1[id]','d2current','v(source)'])
                analysis+='\nlet d2current='+('0*v(o1)' if fault else '@d2[id]')
                rows=reference(name,circuit(fault,d,.5,.5,amplitude,frequency,triangle),analysis,vector,.12)
                times=[r[0] for r in rows]
                def sample(time):
                    hi=bisect.bisect_left(times,time);hi=min(max(hi,1),len(rows)-1);lo=hi-1
                    fraction=(time-times[lo])/(times[hi]-times[lo])
                    return [a+fraction*(b-a) for a,b in zip(rows[lo][1:],rows[hi][1:])]
                selected=[]
                for tick in list(range(0,193))+list(range(640,961)):
                    time=tick/8000;selected.append([case,int(fault),d,amplitude,frequency,int(triangle),time,*sample(time)])
                if maxstep!=.5e-6:
                    coarse=[r for r in transient if r[0]==case and r[1]==int(fault)]
                    refinement=[abs(a-b) for x,y in zip(selected,coarse) for a,b in zip(x[7:30],y[7:30])]
                else:
                    transient.extend(selected)
                    for row in selected:
                        time=round(row[6]*705600)/705600
                        fractional.append([*row[:6],time,*sample(time)])
                    window=[sample(.08+i/768000) for i in range(30720)]
                    for node,label in [(4,'clip'),(11,'jack')]:
                        values=[r[node] for r in window];mean=sum(values)/len(values)
                        harmonics=[2*abs(sum((v-mean)*complex(math.cos(2*math.pi*h*frequency*(.08+i/768000)),-math.sin(2*math.pi*h*frequency*(.08+i/768000))) for i,v in enumerate(values)))/len(values) for h in [1,2,3,5,7]]
                        metrics.append([case,int(fault),d,amplitude,frequency,label,min(values),max(values),mean,*harmonics])
                (out/(name+'.txt')).unlink()
                print(name,'complete',flush=True)
    write(fixtures/'transient.csv',['case','fault','drive','amplitude_V','frequency_hz','triangle','seconds',*NODES,'u1','u2',*[f'C{i}_V' for i in range(1,10)],'D1_A','D2_A','source_V'],transient)
    write(fixtures/'transient-705600.csv',['case','fault','drive','amplitude_V','frequency_hz','triangle','seconds',*NODES,'u1','u2',*[f'C{i}_V' for i in range(1,10)],'D1_A','D2_A','source_V'],fractional)
    write(fixtures/'harmonics.csv',['case','fault','drive','amplitude_V','frequency_hz','node','minimum_V','maximum_V','mean_V','H1_V','H2_V','H3_V','H5_V','H7_V'],metrics)
    netlists=fixtures/'netlists';netlists.mkdir(exist_ok=True)
    for p in out.glob('*.cir'):shutil.copyfile(p,netlists/p.name)
    manifest=dict(simulator=version,source='ElectroSmash TS808 true-bypass drawing87399df; literal independent builtin NPN/diodes, separate continuous op-amp current/capacitor macro',method='Gear2 reltol1e-5/abstol1e-10/vntol1e-7;0.5us maximum step,0.125us hard-fault refinement; uniformly sampled coherent40ms harmonic window',refinement_peak_V=max(refinement),startup_sha256=hashlib.sha256((out/'spinit').read_bytes()).hexdigest(),netlist_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(netlists.glob('*.cir'))},fixture_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(fixtures.glob('*.csv'))},limitations='Nominal generic silicon and reduced4558, ideal9V/4.5V rails, not calibrated hardware. No intrinsic charge/Early/input bias/offset/noise/current limiting. Native SI thermal constants differ slightly from ngspice.')
    (fixtures/'spice-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps({k:v for k,v in manifest.items() if k not in ['netlist_sha256','fixture_sha256']},indent=2))

def compare(actual,expected):
    a=json.loads((actual/'spice-manifest.json').read_text());b=json.loads((expected/'spice-manifest.json').read_text())
    if a['simulator']!=b['simulator'] or a['netlist_sha256']!=b['netlist_sha256']:raise ValueError('Simulator/netlist identity changed')
    peakV=peakA=0
    metadata={'case','fault','control','drive','tone','level','frequency_hz','amplitude_V','triangle','seconds','node'}
    for p in sorted(expected.glob('*.csv')):
        with p.open() as f:old=list(csv.DictReader(f))
        with (actual/p.name).open() as f:new=list(csv.DictReader(f))
        if len(old)!=len(new):raise ValueError('Row count differs:'+p.name)
        for x,y in zip(old,new):
            if x.keys()!=y.keys():raise ValueError('Fields differ:'+p.name)
            for key in x:
                if key in metadata:
                    if x[key]!=y[key]:raise ValueError('Point identity differs:'+key)
                    continue
                u,v=float(x[key]),float(y[key]);current=key.endswith('_A')
                if not math.isclose(u,v,rel_tol=2e-5 if current else 2e-6,abs_tol=1e-10 if current else 5e-7):raise ValueError('Reference differs:'+p.name+':'+key)
                if current:peakA=max(peakA,abs(u-v))
                else:peakV=max(peakV,abs(u-v))
    result=dict(simulator_and_netlists_identical=True,peak_voltage_error_V=peakV,peak_current_error_A=peakA,original_fixtures_unchanged=True)
    (actual/'reproduction-comparison.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ngspice',required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True);p.add_argument('--check-existing',type=Path);a=p.parse_args();generate(a.ngspice,a.output,a.fixtures)
    if a.check_existing:compare(a.fixtures,a.check_existing)
