#!/usr/bin/env python3
"""Independent literal S698 ngspice model and reproducible offline references."""
import argparse,bisect,cmath,csv,hashlib,json,math,shutil,subprocess
from pathlib import Path
from rat_spice_reference import run_spice
NODES=['ip','buf','pre','cu','dp','fbn','dm','dout','clip','pc','cw','ff1','sm','so','tm','tout','lev','bh','byp','out','ref']
AMPS=['xu1a.u','xu1b.u','xu2a.u','xu2b.u']
CAPS=['v(ci,ip)','v(buf,bh)','v(buf,pre)','v(pre,cu)','v(pre,dp)','v(cu,c6low)','v(fbn,dm)','v(dm,dout)','v(dout,c9r)','v(clip,pc)','v(pc,c11r)','v(sm,c12r)','v(sm,so)','v(p2w,tm)','v(tout,co)','v(ff1)','v(ref)']
PI=math.pi
def circuit(g=.5,t=.5,l=.5,rc=0,amp=.15,f=1000,linear=False,triangle=False):
 def r(name,p,n,value):return f'R{name} {p} {n} {value:.17g}\n' if value else f'V{name} {p} {n} 0\n'
 s=f'''Independent S698 Chittum April15 2009 effect-on; fixed nominal pump rails
V9 p9 0 9
.global p9
V18 p18 0 16.2
VN neg 0 -8.6
R29 p9 ref 27k
R30 ref 0 27k
C18 ref 0 47u
Vsrc src 0 SIN(0 {amp:.17g} {f:.17g}) AC 1
Rsource src jackin 1k
R1 jackin ci 10k
C1 ci ip 100n
R2 ip ref 1meg
XU1A ip buf buf amp LO=-3 HI=3
C2 buf bh 4.7u
R3 bh 0 100k
R4 bh byp 560
C3 buf pre 100n
R5 pre cu 5.1k
C4 pre cu 68n
R6 pre dp 10k
C5 pre dp 68n
R7 pre ff1 1.5k
C16 ff1 0 1u
R19 ff1 sm 15k
R8 cu ref 1.5k
C6 cu c6low 390n
R9 c6low ref 1k
'''
 s+=r('P1Btop','dp','ref',100000*g)+r('P1Bbottom','pb','ref',100000*(1-g))
 s+='''R10 pb fbn 2k
R11 fbn dm 15k
C7 fbn dm 82n
R12 dm dout 422k
C8 dm dout 390p
XU1B dp dm dout amp LO=-3 HI=3
C9 dout c9r 1u
R13 c9r clip 1k
D2 0 clip Ge
D3 clip 0 Ge
C10 clip pc 1u
R16 pc sm 47k
C11 pc c11r 2.2n
R15 c11r cw 22k
R17 sm cw 27k
C12 sm c12r 27n
R18 c12r cw 12k
'''
 s+=r('P1Atop','cu','track',100000*g)+r('P1Abottom','track','ref',100000*(1-g))+r('P1Acontact','track','cw',rc)
 s+='''R20 sm so 392k
C13 sm so 820p
XU2A ref sm so amp LO=-11.6 HI=10.2
R21 so p2b 1.8k
R22 so tm 100k
R23 p2t tout 4.7k
R241 tm tout 100k
'''
 s+=r('P2top','p2t','p2w',10000*t)+r('P2bottom','p2w','p2b',10000*(1-t))
 s+='''C14 p2w tm 3.9n
XU2B ref tm tout amp LO=-11.6 HI=10.2
C15 tout co 4.7u
R25 co lev 560
'''
 s+=r('P3top','lev','out',10000*(1-l))+r('P3bottom','out','0',10000*l)
 s+='''R26 byp out 68k
R27 lev out 68k
R28 out 0 100k
Rload out 0 1meg
.model Ge D(IS=600n N=1.5 EG=.67 CJO=0 TT=0 TNOM=26.85)
.subckt amp p n o params: LO=-3 HI=3
Rin p n 1e12
Bstate 0 u I=1e-6*(min(max(94.2477796076938*(200000*v(p,n)-v(u)),-13e6),13e6)-1e10*(v(u)-min(max(v(u),LO),HI)))
Cstate u 0 1u
Ecenter center 0 p9 0 .5
Edrive internal center u 0 1
Rout internal o 50
.ends
'''
 s+='\n.nodeset '+ ' '.join('v('+a+'.u)=0 v('+a+'.internal)=4.5' for a in ['xu1a','xu1b','xu2a','xu2b'])+'\n.nodeset '+ ' '.join('v('+n+')=4.5' for n in ['ref','ip','buf','pre','cu','dp','ff1','sm','c6low','pb','fbn','dm','dout','pc','c11r','cw','c12r','track','so','tm','p2t','p2b','p2w','tout'])+'\n'
 if triangle:s=s.replace(f'SIN(0 {amp:.17g} {f:.17g}) AC 1',f'0 AC 1\nBtriangle src 0 V={amp:.17g}*2/pi*asin(sin(2*pi*{f:.17g}*time))').replace('Vsrc src 0 0 AC 1','Vsrc disconnected 0 0 AC 1')
 if linear:
  s='\n'.join(line for line in s.splitlines() if not line.startswith('.nodeset'))+'\n'
  a=s.index('Bstate');b=s.index('Cstate',a);s=s[:a]+'Gerr 0 u p n 18.8495559215388\nRstate u 0 10610.3295394597\n'+s[b:]
 else:s=s.replace('.nodeset','.ic')
 return s
def stimulus(t,amp,hz,triangle):
    return amp*(2/PI*math.asin(math.sin(2*PI*hz*t)) if triangle else math.sin(2*PI*hz*t))

def currents(v):
    n=dict(zip(NODES+AMPS+['c11r','c12r'],v));i11=(n['cw']-n['c11r'])/22000
    return [(n['pc']-n['sm'])/47000-i11,(n['ff1']-n['sm'])/15000,i11+(n['cw']-n['sm'])/27000+(n['cw']-n['c12r'])/12000]

def interpolate(rows,t):
    k=bisect.bisect_left(rows,t,key=lambda r:r[0]);k=max(1,min(k,len(rows)-1));a,b=rows[k-1],rows[k]
    q=(t-a[0])/(b[0]-a[0]);return [a[i]+q*(b[i]-a[i]) for i in range(1,len(a))]

def write(path,header,rows):
    with path.open('w',newline='') as f:w=csv.writer(f);w.writerow(header);w.writerows(rows)

def generate(exe,out,fixtures,grid_rates=None,selected_cases=None):
    out.mkdir(parents=True,exist_ok=True);fixtures.mkdir(parents=True,exist_ok=True)
    (out/'spinit').write_text('* Explicit SF011 startup\nset num_threads=1\n')
    version=subprocess.check_output([exe,'--version'],text=True).strip();netlists=fixtures/'netlists';netlists.mkdir(exist_ok=True)
    def run(name,c,analysis,vec):
        options='option reltol=1e-5 abstol=1e-10 vntol=1e-7' if 'tran ' in analysis else 'option reltol=1e-9 abstol=1e-15 vntol=1e-12'
        rows=run_spice(exe,out,name,c,options+'\n'+analysis,vec)
        log=(out/(name+'.log')).read_text();assert not any(s in log.lower() for s in ['aborted','timestep too small','doanalyses:']),name
        assert rows and all(math.isfinite(v) for row in rows for v in row),name
        shutil.copyfile(out/(name+'.cir'),netlists/(name+'.cir'))
        (out/(name+'.txt')).unlink() # own temporary dense data; fixtures/logs/netlists retained
        return rows
    vector=' '.join('v('+n+')' for n in NODES+AMPS+['c11r','c12r'])
    bias=[]
    for g in [0,.05,.5,.95,1]:
        for rc in [0,10000,100000,1000000,1e9]:
            row=run(f'bias-g{g}-rc{rc}',circuit(g=g,rc=rc,linear=True),'op',' '.join('v('+n+')' for n in NODES+AMPS)+' '+' '.join(CAPS))[0][1:]
            expected=[0 if n in ['clip','lev','bh','byp','out'] else 4.5 for n in NODES]+[0]*4
            assert max(abs(a-b) for a,b in zip(row,expected))<1e-5,(g,rc,row)
            bias.append([g,rc,*row])
    write(fixtures/'bias.csv',['gain','contact_ohms',*NODES,*AMPS,*('C'+str(k)+'_V' for k in range(1,18))],bias)
    model='Independent generic nominal Ge diode\nVtest d 0 0\nDtest d 0 Ge\n.model Ge D(IS=600n N=1.5 EG=.67 CJO=0 TT=0 TNOM=26.85)\n'
    diode=run('diode',model,'dc Vtest -.5 .5 .01','i(Vtest)')
    write(fixtures/'diode.csv',['V','anode_current_A'],[[r[0],-r[1]] for r in diode])
    controls=[(g,.5,.5,f) for f in [False,True] for g in [0,.05,.5,.95,1]]
    controls +=[(.5,t,.5,f) for f in [False,True] for t in [0,1]]+[(.5,.5,l,f) for f in [False,True] for l in [0,1]]
    ac=[];candidates=[]
    for g,t,l,f in controls:
        rows=run(f'ac-g{g}-t{t}-l{l}-f{int(f)}',circuit(g=g,t=t,l=l,rc=100000*int(f),linear=True),'ac dec 10 100 10000',vector)
        for row in rows:
            values=[complex(*row[i:i+2]) for i in range(1,len(row),2)];mix=currents(values);n=dict(zip(NODES+AMPS+['c11r','c12r'],values))
            fb=(n['so']-n['sm'])*(1/392000+2j*PI*row[0]*820e-12)
            assert abs(sum(mix)+fb-(n['sm']-n['ref'])/1e12)<2e-10,(g,t,l,f,row[0])
            v=values[:len(NODES)+4]+mix
            ac.append([g,t,l,int(f),row[0],*[p for value in v for p in [value.real,value.imag]]])
    write(fixtures/'ac.csv',['gain','treble','level','fault','hz',*[n+'_'+part for n in NODES+AMPS+['drive_A','clean1_A','clean2_A'] for part in ['real','imag']]],ac)
    for g in [0,.5,1]:
        for rc in [0,10000,100000,1000000,1e9]:
            rows=run(f'candidate-g{g}-rc{rc}',circuit(g=g,rc=rc,linear=True),'ac dec 10 100 10000',vector)
            for row in rows:
                values=[complex(*row[i:i+2]) for i in range(1,len(row),2)];v=values[:len(NODES)+4]+currents(values)
                candidates.append([g,rc,row[0],*[p for value in v for p in [value.real,value.imag]]])
    write(fixtures/'candidate.csv',['gain','contact_ohms','hz',*[n+'_'+part for n in NODES+AMPS+['drive_A','clean1_A','clean2_A'] for part in ['real','imag']]],candidates)
    cases=[('gain0',0,.15,1000,False),('push',.05,.15,1000,False),('blend',.5,.15,1000,False),('high',.95,.15,1000,False),('gain1',1,.15,1000,False),('low',.5,.005,1000,False),('medium',.5,.05,1000,False),('hard',.5,.35,1000,False),('slew',1,.8,10000,False),('triangle',.5,.3,1000,True)]
    if selected_cases:
        assert set(selected_cases)<=set(c[0] for c in cases)
        cases=[c for c in cases if c[0] in selected_cases]
    grids={rate:[] for rate in (grid_rates or [529200,576000,768000,1152000])};harmonics=[];refinement=[];refinement_by_node={}
    for name,g,amp,hz,triangle in cases:
        for fault in [False,True]:
            c=circuit(g=g,rc=100000*int(fault),amp=amp,f=hz,triangle=triangle)
            rows=run(f'transient-{name}-f{int(fault)}',c,'option method=gear maxord=2\ntran 1u .06 0 .5u uic',vector)
            assert rows[-1][0]>=.06-1e-10 and rows[0][0]<=1e-8,(name,rows[0][0],rows[-1][0])
            # Analytical quiescent DC is independently solved above. UIC releases
            # consistent capacitor initial conditions, never holds a node in time.
            rows.insert(0,[0,*[0 if n in ['clip','lev','bh','byp','out'] else 4.5 for n in NODES],*[0]*4,4.5,4.5])
            for rate,outrows in grids.items():
                indices=sorted(set([int(i*.06*rate/256) for i in range(257)]+list(range(25))+[int(.059*rate)+4*i for i in range(int(.001*rate)//4)]))
                for index in indices:
                    time=index/rate;values=interpolate(rows,time);outrows.append([name,int(fault),g,.5,.5,amp,hz,int(triangle),time,stimulus(time,amp,hz,triangle),*values[:len(NODES)+4],*currents(values)])
            sample_rate=768000;start=.04;count=int(.02*sample_rate)
            samples=[interpolate(rows,start+k/sample_rate) for k in range(count)]
            signals=[[v[NODES.index(node)] for v in samples] for node in ['clip','so','out']]
            signals += [[currents(v)[k] for v in samples] for k in range(3)]
            for label,signal in zip(['clip','so','out','drive_A','clean1_A','clean2_A'],signals):
                result=[]
                for h in [1,2,3,5,7]:
                    result.append(abs(sum(value*cmath.exp(-2j*PI*h*hz*(start+i/sample_rate)) for i,value in enumerate(signal)))*2/count)
                harmonics.append([name,int(fault),label,hz,min(signal),max(signal),sum(signal)/count,*result])
            if name=='slew' and fault:
                fine=run('refinement-slew-f1',c,'option method=gear maxord=2\ntran 1u .06 0 .125u uic',vector)
                assert fine[-1][0]>=.06-1e-10
                differences=[[abs(a-b) for a,b in zip(interpolate(rows,.04+k/768000),interpolate(fine,.04+k/768000))] for k in range(count)]
                refinement=[max(r) for r in differences]
                refinement_by_node={name:max(r[j] for r in differences) for j,name in enumerate(NODES+AMPS+['c11r','c12r'])}
            print('complete',name,int(fault),flush=True)
    from klon_supply_reference import audit
    audit(run,circuit,write,fixtures)
    header=['case','fault','gain','treble','level','amplitude','frequency_hz','triangle','time_s','input_V',*NODES,*AMPS,'drive_A','clean1_A','clean2_A']
    for rate,rows in grids.items():write(fixtures/f'transient-{rate}.csv',header,rows)
    write(fixtures/'harmonics.csv',['case','fault','node','hz','min','max','mean','H1','H2','H3','H5','H7'],harmonics)
    manifest=dict(simulator=version,source='Chittum April15 2009 S698, SHA20f7363; builtin Ge, independent continuous TL072 reduced macro; DC/AC equivalent explicit G/R/C linearization',method='Gear2 reltol1e-5 abstol1e-10 vntol1e-7;0.5us max step,hard-fault0.125us refinement;768kHz coherent20ms harmonic window',refinement_peak_V=max(refinement),refinement_peak_by_node_V=refinement_by_node,fixture_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(fixtures.glob('*.csv'))},netlist_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(netlists.glob('*.cir'))},limitations='Nominal generic Ge and reduced legacy TL072; fixed annotated pump rails, actual passive Vref. No physical calibration, switching pump/sag/noise/offset/common-mode reversal/device charge/current limit/recovery. PWL behavioral OP pseudo-transients were rejected; accepted DC/AC uses exactly equivalent unsaturated explicit G/R/C, full nonlinear time uses verified DC capacitor equilibrium/UIC.')
    (fixtures/'spice-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')

def compare(generated,existing):
    errors={}
    a=json.loads((generated/'spice-manifest.json').read_text());b=json.loads((existing/'spice-manifest.json').read_text());assert a['simulator']==b['simulator'] and a['netlist_sha256']==b['netlist_sha256']
    for name in b['fixture_sha256']:
        with (generated/name).open() as f:one=list(csv.reader(f))
        with (existing/name).open() as f:two=list(csv.reader(f))
        assert len(one)==len(two) and one[0]==two[0],name;peak=0
        for r,s in zip(one[1:],two[1:]):
            for x,y in zip(r,s):
                try:v,w=float(x),float(y)
                except ValueError:assert x==y;continue
                peak=max(peak,abs(v-w));assert abs(v-w)<=1e-10+1e-7*abs(w),(name,x,y)
        errors[name]=peak
    print(json.dumps(dict(regenerated=True,peak_numeric_differences=errors),indent=2))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ngspice',default=shutil.which('ngspice'));p.add_argument('--output',type=Path,required=True);p.add_argument('--work',type=Path,required=True);p.add_argument('--check-existing',type=Path);p.add_argument('--grids',type=int,nargs='+');p.add_argument('--cases',nargs='+');a=p.parse_args()
    if not a.ngspice:p.error('ngspice required')
    generate(a.ngspice,a.work.resolve(),a.output.resolve(),a.grids,a.cases)
    if a.check_existing:compare(a.output.resolve(),a.check_existing.resolve())
