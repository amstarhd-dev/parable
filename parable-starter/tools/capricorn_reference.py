#!/usr/bin/env python3
"""Compact independent ngspice checks; reuses the existing Soundforge runner."""
import argparse,csv,hashlib,json,subprocess
from pathlib import Path
from rat_spice_reference import run_spice
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ngspice',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
(out/'spinit').write_text('* SF018 explicit startup\nset num_threads=1\n')
model='.model N NPN(IS=1e-14 BF=350 BR=4 TNOM=26.85)\n'
base='''Independent1978-family Q2 reduction
Vcc rail 0 9
R6 b 0 100k
R7 c b 470k
R8 rail c 10k
R9 e 0 22
Q2 c b e N
'''+model
row=run_spice(a.ngspice,out,'bias',base,'op','v(b) v(c) v(e)')[0];bias=row[-3:];b,c,e=bias
curve=run_spice(a.ngspice,out,'curve',base+f'Vinput input 0 {b}\nRsource input b 550\nVload load 0 {c}\nRload c load 100k\n',f'dc Vinput {b-.2} {b+.2} .05','v(c)')
curves=[[r[0]-b,r[1]-c] for r in curve]
ac=[]
for fault in [0,1]:
    circuit=base+f'''Vin in 0 AC 1
Rsource in ci 550
C3 ci b 47n
Cload c cl 1
Rload cl 0 100k
'''+('' if fault else 'C4 c b 250p\n')
    for hz in [100,1000,6000,12000]:
        r=run_spice(a.ngspice,out,f'q2-{fault}-{hz}',circuit,f'ac lin 1 {hz} {hz}','v(c)')[0];ac.append([fault,hz,r[1],r[2]])
def tone(tone,companion=False,diodes=False):
    w='l' if tone==0 else 'h' if tone==1 else 'w';s='Independent loaded passive Tone/Level\nVin input 0 AC 1\n'
    if companion:s+='R14 input x 2200\nRc9 x d '+str(1/(2*192000*.47e-6))+'\nRc10 d 0 '+str(1/(2*192000*10e-9))+'\nRc11 d z '+str(1/(2*192000*22e-9))+'\nRc12 l 0 '+str(1/(2*192000*100e-9))+'\n'
    else:s+='Vwire input d 0\nC11 d z 22n\nC12 l 0 100n\n'
    s+='R15 z h 2200\nR17 h 0 6800\nR16 d l 6800\n'
    if tone<1:s+=f'Rbright h {w} {20000*(1-tone)}\n'
    if tone>0:s+=f'Rdark l {w} {20000*tone}\n'
    s+=f'Rleveltop {w} jack 20000\nRlevelbottom jack 0 80000\nRbuffer jack 0 1meg\n'
    if diodes:s+='.model D D(IS=2.52e-9 N=1.752 TNOM=26.85)\nDpositive d 0 D\nDnegative 0 d D\n'
    return s
responses=[]
for knob in [0,.5,1]:
    for hz in [100,1000,6000]:
        r=run_spice(a.ngspice,out,f'tone-{knob}-{hz}',tone(knob),f'ac lin 1 {hz} {hz}','v(jack)')[0];responses.append([knob,hz,r[1],r[2]])
# Instantaneous trapezoidal companions, zero histories. Independent SPICE finds
# open-circuit drive and diode-loaded voltage, not C++-generated expectations.
openrows=run_spice(a.ngspice,out,'clip-open',tone(.5,True), 'dc Vin -32 32 8','v(d)')
closed=run_spice(a.ngspice,out,'clip-diodes',tone(.5,True,True),'dc Vin -32 32 8','v(d)')
clips=[[x[1],y[1]] for x,y in zip(openrows,closed)]
for name,header,rows in [('bias.csv',['base_V','collector_V','emitter_V'],[bias]),('curve.csv',['input_V','collector_delta_V'],curves),('q2-ac.csv',['C4_open','Hz','real','imag'],ac),('tone.csv',['Tone','Hz','real','imag'],responses),('clip.csv',['open_source_V','diode_V'],clips)]:
    with (out/name).open('w') as f:w=csv.writer(f);w.writerow(header);w.writerows(rows)
manifest=dict(simulator=subprocess.check_output([a.ngspice,'--version'],text=True).strip(),simulator_sha256=hashlib.sha256(Path(a.ngspice).read_bytes()).hexdigest(),source='Builtin ngspice transistor/diode laws and separate passive node solves. Explicit reduction, not calibrated1978 hardware.',generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixtures={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.iterdir()) if p.suffix in ['.cir','.csv']})
(out/'reference.json').write_text(json.dumps(manifest,indent=2)+'\n');print('Generated1bias,9static,8feedback,9Tone,9clip rows; no large transient campaign')
