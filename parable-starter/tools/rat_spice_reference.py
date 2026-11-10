#!/usr/bin/env python3
"""Offline independent ngspice reduced-circuit reference; never firmware DSP."""
import argparse, bisect, csv, hashlib, json, math, os, subprocess
from pathlib import Path

def pot(x): return (20**x-1)/19

def run_spice(exe, output, name, circuit, analysis, vector):
    raw=output/(name+'.txt')
    netlist=circuit+f'''\n.options reltol=1e-8 abstol=1e-13 vntol=1e-10 gmin=1e-14
.temp 26.85
.control
set noaskquit
set wr_singlescale
set wr_vecnames
{analysis}
wrdata {raw.name} {vector}
quit
.endc
.end
'''
    path=output/(name+'.cir');path.write_text(netlist)
    env=dict(os.environ,SPICE_SCRIPTS=str(output.resolve()))
    result=subprocess.run([exe,'-n','-b',str(path.resolve())],cwd=output.resolve(),env=env,capture_output=True,text=True,timeout=90)
    (output/(name+'.log')).write_text(result.stdout+result.stderr)
    if result.returncode or not raw.exists(): raise RuntimeError(f'ngspice failed: {name}')
    return [[float(x) for x in row.split()] for row in raw.read_text().splitlines()[1:]]

def generate(exe, output, fixtures):
    output.mkdir(parents=True,exist_ok=True);fixtures.mkdir(parents=True,exist_ok=True)
    # No host/user startup scripts or unpinned external device models.
    (output/'spinit').write_text('* Explicit LabRAT reference startup\nset num_threads=1\n')
    version=subprocess.check_output([exe,'--version'],text=True).strip()
    ac_rows=[]
    for case in ['healthy','ruetz','compensation']:
        for distortion in [.02,.35,.9]:
            rf=100000*pot(distortion);cf=1e-7 if case=='compensation' else 100e-12
            circuit=f'''Independent RAT input + finite-open-loop feedback AC reference
Vin source 0 AC 1
Cin source bias 22n
Rbias bias 0 1Meg
Rin bias input 1k
Crf input 0 1n
Edrive drive 0 input inv 200000
Rdominant drive pole 1
Cdominant pole 0 {200000/(2*math.pi*1e6):.17g}
Eout out 0 pole 0 1
Rfeedback out inv {rf:.17g}
Cfeedback out inv {cf:.17g}
Rb inv bass 560
Cb bass 0 4.7u
'''
            if case!='ruetz': circuit+='Rt inv treble 47\nCt treble 0 2.2u\n'
            rows=run_spice(exe,output,f'ac-{case}-{distortion}',circuit,'ac dec 12 20 20000','v(out)')
            ac_rows += [[case,distortion,*r] for r in rows]
    with (fixtures/'preamp-ac.csv').open('w',newline='') as f:
        w=csv.writer(f,lineterminator="\n");w.writerow(['case','distortion','frequency_hz','real','imag']);w.writerows(ac_rows)
    transient=[]
    for case in ['healthy','diode_open','filter_dark']:
        filter_control=1 if case=='filter_dark' else .5
        circuit=f'''Independent RAT passive clipping/filter/output transient reference
Vsrc source 0 SIN(0 3 997)
Ccouple source pre 4.7u
Rclip pre clip 1k
'''
        if case!='diode_open':circuit+='Dpositive clip 0 silicon\n'
        circuit+=f'''Dnegative 0 clip silicon
.model silicon D(IS=2.52n N=1.752)
Rfilter clip tone {1500+100000*pot(filter_control):.17g}
Cfilter tone 0 3.3n
Cgate tone gate 22n
Rgate gate 0 1Meg
Ebuffer buffered 0 gate 0 1
Cout buffered output 10u
Rout output top 10k
Rtop top wiper 30k
Rbottom wiper 0 70k
Rload wiper 0 1Meg
'''
        rows=run_spice(exe,output,'transient-'+case,circuit,'tran 0.5u 0.204 0.2 0.5u','v(clip) v(tone) v(wiper)')
        times=[r[0] for r in rows]
        for sample in range(38401,39169,4):
            t=sample/192000
            hi=bisect.bisect_left(times,t);lo=max(0,hi-1);hi=min(hi,len(rows)-1)
            if not times[lo]<=t<=times[hi]:raise RuntimeError('Missing transient reference span')
            frac=(t-times[lo])/(times[hi]-times[lo]) if hi!=lo else 0
            values=[rows[lo][j]+frac*(rows[hi][j]-rows[lo][j]) for j in range(1,4)]
            transient.append([case,sample,t,*values])
    with (fixtures/'clip-transient.csv').open('w',newline='') as f:
        w=csv.writer(f,lineterminator="\n");w.writerow(['case','sample_192khz','seconds','clip_volts','tone_volts','wiper_volts']);w.writerows(transient)
    manifest={'simulator':version,'kind':'independent simulator of reduced circuit, not a verified full LM308 macromodel',
              'startup_sha256':hashlib.sha256((output/'spinit').read_bytes()).hexdigest(),
              'netlist_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(output.glob('*.cir'))},
              'fixtures_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(fixtures.glob('*.csv'))}}
    (fixtures/'spice-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Created independent AC and diode/filter transient references:',fixtures)
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ngspice',default='ngspice');p.add_argument('--output',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True)
    a=p.parse_args();generate(a.ngspice,a.output,a.fixtures)
