#!/usr/bin/env python3
"""Small independent builtin-BJT ngspice reference for the declared reduction."""
import argparse,csv,hashlib,json,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--ngspice',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
models='.model N NPN(IS=1e-14 BF=400 BR=4 TNOM=26.85)\n.model P PNP(IS=1e-9 BF=100 BR=4 TNOM=26.85)\n.temp 26.85\n'
def run(name,circuit,control):
    f=out/(name+'.cir');f.write_text('SF017 independent reduced reference\n'+circuit+models+'.control\nset numdgt=15\n'+control+'\nquit\n.endc\n.end\n')
    r=subprocess.run([str(a.ngspice.resolve()),'-b',str(f)],capture_output=True,text=True)
    (out/(name+'.log')).write_text(r.stdout+r.stderr)
    if r.returncode or 'Error:' in r.stderr:raise RuntimeError(name+': '+r.stdout+r.stderr)
    return r.stdout
def number(text,key):
    m=re.search(re.escape(key)+r'\s*=\s*([-+\d.eE]+)',text)
    if not m:raise ValueError(key+' missing: '+text)
    return float(m.group(1))
def base(fault):
    return f'''Vrail va 0 9
R11 va vc 1k
R12 vc vd 56k
R13 vd 0 33k
R1 vd b 220k
R3 vc x {10000 if fault else 5600}
R2 e f 5.6k
Rtrim f 0 68
R7 va ep 4.7k
R6 y 0 5.6k
Q1 x b e N
Q2 y x ep P
Ileak ep y 50u
'''
def dynamic(fault,gain,frequency,amp=None):
    signal='AC 1' if amp is None else f'SIN(0 {amp} {frequency})'
    return base(fault)+f'''Vin in 0 {signal}
Rsource in ci 1k
C2 ci b 1u
C3 b 0 330p
C4 e f 1
C6 y fb 1
R8_GAIN f fb {5600+250000*gain}
C7 f fb 1n
C8 ep va 330n
Rlow ep lc 3500.65
C9 lc va 330u
C11 vc 0 1
C12 vd 0 1
'''
biases={};biasrows=[];curves=[];frequencies=[];impedances={}
for fault in [0,1]:
    text=run(f'bias-{fault}',base(fault),'op\nprint v(b) v(x) v(e) v(ep) v(y) v(vc) v(vd)')
    v=[number(text,'v('+n+')') for n in ['b','x','e','ep','y','vc','vd']];biases[fault]=v;biasrows.append([fault,*v])
    b,x,e,ep,y,vc,vd=v
    for gain in [.1,.9]:
        reff=1/(1/4700+1/3500.65);emitter_anchor=e-68*e/5668;ep_anchor=ep+reff*(9-ep)/4700
        circuit=f'''Vsig source 0 {b}
Vvc vc 0 {vc}
Vvd vd 0 {vd}
Ve ve 0 {emitter_anchor}
Vep vep 0 {ep_anchor}
Rsource source b 1k
R1 vd b 220k
R3 vc x {10000 if fault else 5600}
Re e ve 68
Rep ep vep {reff}
R6 y 0 5600
Voffset y fb {y-e}
Rfeedback fb e {5600+250000*gain}
Q1 x b e N
Q2 y x ep P
Ileak ep y 50u
Iprobe 0 y 0
'''
        path=out/f'curve-{fault}-{gain}.txt'
        run(f'curve-{fault}-{gain}',circuit,f'set wr_singlescale\nset wr_vecnames\ndc Vsig {b-.1} {b+.1} .025\nwrdata {path} v(y)')
        for line in path.read_text().splitlines()[1:]:
            values=list(map(float,line.split()));curves.append([fault,gain,values[0]-b,values[1]-y])
        text=run(f'impedance-{fault}-{gain}',circuit,'tf v(y) Vsig\nprint all')
        z=number(text,'output_impedance_at_v(y)');impedances[fault,gain]=z
    for hz in [100,1000,5000]:
        text=run(f'ac-{fault}-{hz}',dynamic(fault,.5,hz),f'ac lin 1 {hz} {hz}\nprint real(v(y)) imag(v(y))')
        frequencies.append([fault,.5,hz,number(text,'real(v(y))'),number(text,'imag(v(y))')])
# One small-signal loaded transient: nonlinear core retained, independent BJT and
# full small-signal capacitors, ideal buffer isolates the stated Thevenin source.
fault=0;gain=.1;b,x,e,ep,y,vc,vd=biases[fault];ro=impedances[fault,gain]
level=.8;bottom=level*100000/(1+level*100000/1e6);pot=(1-level)*100000+bottom
circuit=dynamic(fault,gain,1000,1e-4)+f'''Bbuffer buf 0 V=v(y)-{y}
Rout buf c10 {ro}
C10 c10 r9 100u
R9 r9 primary 15k
R10 primary 0 33k
Rp primary core 820
Rsecondary core secondary 1070
Rleveltop secondary jack {(1-level)*100000}
Rlevelbottom jack 0 {level*100000}
Rexternal jack 0 1meg
Cflux phi 0 1 IC=0
Bflux 0 phi I=v(core)
Bmag core 0 I=v(phi)/20+v(phi)*v(phi)*v(phi)/(20*.008*.008)
'''
path=out/'transient.txt'
run('transient',circuit,f'set wr_singlescale\nset wr_vecnames\ntran .5u .03 0 .5u\nwrdata {path} v(jack) v(phi)')
raw=[list(map(float,line.split())) for line in path.read_text().splitlines()[1:]]
targets=[.001,.003,.008,.012,.02,.03];trans=[]
for t in targets:
    closest=min(raw,key=lambda r:abs(r[0]-t));trans.append([closest[0],closest[1],closest[2]])
for name,header,rows in [('bias.csv',['fault','B1','C1_B2','E1','E2','C2','VC','VD'],biasrows),('curve.csv',['fault','gain','input_V','preamp_V'],curves),('ac.csv',['fault','gain','Hz','real','imag'],frequencies),('transient.csv',['time_s','jack_V','flux_Vs'],trans)]:
    with (out/name).open('w') as f:w=csv.writer(f);w.writerow(header);w.writerows(rows)
manifest={'simulator':subprocess.check_output([str(a.ngspice.resolve()),'--version'],text=True),'executable_sha256':hashlib.sha256(a.ngspice.read_bytes()).hexdigest(),'source':'builtin ngspice BJTs, separate node solve; C4/C6 and supply decoupling1F implement explicit AC-short reduction; not a whole-Hudson calibration','rows':{'bias':len(biasrows),'curve':len(curves),'ac':len(frequencies),'transient':len(trans)},'output_impedance':{str(k):v for k,v in impedances.items()},'sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.suffix in {'.csv','.cir'}}}
(out/'reference.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Independent reduced reference:',manifest['rows'])
