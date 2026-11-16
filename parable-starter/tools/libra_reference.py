#!/usr/bin/env python3
"""Independent nodal SPICE equivalent of the declared fixed-VBE/beta reduction."""
import argparse,csv,hashlib,json,math,subprocess
from pathlib import Path
from rat_spice_reference import run_spice
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ngspice',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);(out/'spinit').write_text('* Explicit SF013 startup\nset num_threads=1\n')
def circuit(fault,feedback,load=1000,transient=False):
    s=f'''Independent classAB reduced circuit; no C++ affine coefficients
Vp railp 0 4.5
Vn railn 0 -4.5
Vin source 0 {'SIN(0 .003 220)' if transient else '0'}
'''
    if transient:s+=f'Cin source filtered 10n\nRin filtered 0 1meg\nBpre input 0 V=max(-4,min(4,{4*20**.1}*v(filtered)))\n'
    else:s+='Vinputwire source input 0\n'
    s+=f'''RB1 railp bn {270000 if fault else 27000}
RB2 bn u 4700
RB3 u bp 4700
RB4 bp railn 27000
VmeasureN bn bin 0
BbeN bin en I=max((v(bin,en)-.64)/.01,0)
FcollectorN railp en VmeasureN 100
ReN en output 2.2
VmeasureP bip bp 0
BbeP ep bip I=max((v(ep,bip)-.64)/.01,0)
FcollectorP ep railn VmeasureP 100
ReP output ep 2.2
Rload output 0 {load}
Rf output feedback 30000
Rg feedback 0 10000
Btarget target 0 V=40*(v(input){'-v(feedback)' if feedback else ''})
Rdriver target state 1000
Bdriver u 0 V=v(state)
Cdriver state 0 {1/(2*math.pi*500*1000)}
Brail state 0 I=1e6*(v(state)-max(-3.5,min(3.5,v(state))))
Bvolume level 0 V=.8*v(output)
Cout level jack 1u
Rjack jack 0 100k
'''
    return s
bias=[];curves=[];overload=[];loads=[];trans=[]
vector='v(u) v(output) i(VmeasureN) i(VmeasureP)'
for fault in [0,1]:
 for feedback in [0,1]:
    name=f'dc-{fault}-{feedback}';rows=run_spice(a.ngspice,out,name,circuit(fault,feedback),'dc Vin -.04 .04 .01',vector)
    for r in rows:curves.append([fault,feedback,r[0],r[1],r[2],101*r[3],101*r[4]])
    r=min(rows,key=lambda r:abs(r[0]));bias.append([fault,feedback,*r[1:3],101*r[3],101*r[4]])
 for load in [100,10000]:
    rows=run_spice(a.ngspice,out,f'load-{fault}-{load}',circuit(fault,1,load),'dc Vin .05 .05 .01',vector)
    for r in rows:loads.append([fault,load,.05,r[1],r[2],101*r[3],101*r[4]])
 rows=run_spice(a.ngspice,out,f'overload-{fault}',circuit(fault,1),'dc Vin -1.2 1.2 .4',vector)
 for r in rows:overload.append([fault,r[0],r[1],r[2],101*r[3],101*r[4]])
 rows=run_spice(a.ngspice,out,f'transient-{fault}',circuit(fault,1,transient=True),'tran 2u .02 0 2u','v(u) v(output) v(jack)')
 for t in [.001,.003,.005,.009,.013,.017,.02]:
    r=min(rows,key=lambda r:abs(r[0]-t));trans.append([fault,*r])
for name,header,rows in [('bias.csv',['fault','feedback','driver_V','combined_V','upper_A','lower_A'],bias),('curve.csv',['fault','feedback','input_V','driver_V','combined_V','upper_A','lower_A'],curves),('load.csv',['fault','load_ohms','input_V','driver_V','combined_V','upper_A','lower_A'],loads),('overload.csv',['fault','input_V','driver_V','combined_V','upper_A','lower_A'],overload),('transient.csv',['fault','time_s','driver_V','combined_V','jack_V'],trans)]:
 with (out/name).open('w') as f:w=csv.writer(f);w.writerow(header);w.writerows(rows)
manifest=dict(reference='Independent SPICE resistor network, base-junction piecewise0.64V conduction with0.01ohm on resistance and separate beta100 current-controlled collectors. Not a calibrated builtin2N3904/2N3906 model.',simulator=subprocess.check_output([a.ngspice,'--version'],text=True).strip(),simulator_sha256=hashlib.sha256(Path(a.ngspice).read_bytes()).hexdigest(),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixtures={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.iterdir()) if p.suffix in ['.csv','.cir']})
(out/'reference.json').write_text(json.dumps(manifest,indent=2)+'\n');print('Small independent equivalent generated:4bias,36transfer,4load,14overload,14transient samples')
