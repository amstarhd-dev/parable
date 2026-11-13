#!/usr/bin/env python3
"""Independent built-in ngspice BJT/diode solve of one pinned V3 drawing.
Never consumes native output or native stamps. Uses existing run_spice wrapper.
"""
import argparse
import bisect
import csv
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess
from rat_spice_reference import run_spice

MODEL='''.model silicon NPN(IS=1e-14 BF=300 BR=4 NF=1 NR=1 VAF=0 VAR=0 IKF=0 IKR=0 CJE=0 CJC=0 TF=0 TR=0 TNOM=26.85)
.model pair D(IS=2.52n N=1.752 RS=0 CJO=0 TT=0 TNOM=26.85)
'''
NODES=['b4','c4','e4','b3','c3','e3','b2','c2','e2','b1','c1','e1','sw','low','high','d3','d2']


def pulse(t):
    return f'((time>{t})&&(time<{t+.05:.17g}) ? 0.7*sin(2*PI*110*(time-{t}))*sin(PI*(time-{t})/0.05)^2 : 0)'


def circuit(fault=False,sustain=.75,tone=.5,volume=.5,attack=True,rf=None):
    def resistance(name,p,n,value):
        return f'R{name} {p} {n} {value:.17g}\n' if value else f'V{name} {p} {n} 0\n'
    source='0.02*sin(2*PI*223*(time-0.08))*(time>0.08)'
    if attack:source+=' + '+pulse(.02)+' + '+pulse(.22)
    return f'''ElectroSmash V3 drawing522df7b5, factual topology; no artwork import
Vcc rail 0 9
Bsrc drive 0 V={{{source}}}
Vsignal source drive 0 AC 1
Rexternal source in 1k
R2 in ci 39k
C1 ci b4 1u
Q4 c4 b4 e4 silicon
R13 rail c4 10k
R14 b4 0 47k
R9 b4 c4 470k
R22 e4 0 100
C10 b4 c4 470p
C4 c4 st 1u
''' + resistance('su','st','sw',(1-sustain)*1e5)+resistance('sl','sw','sb',sustain*1e5)+f'''R23 sb 0 1k
C5 sw i3 100n
R19 i3 b3 10k
Q3 c3 b3 e3 silicon
R18 rail c3 10k
R20 b3 0 100k
R17 b3 c3 470k
R21 e3 0 150
C12 b3 c3 470p
C6 b3 d3 1u
D3 d3 c3 pair
D4 c3 d3 pair
C13 c3 i2 100n
R12 i2 b2 10k
Q2 c2 b2 e2 silicon
R11 rail c2 10k
R16 b2 0 100k
R15 b2 c2 {rf if rf is not None else 2.2e6 if fault else 470e3:.17g}
R10 e2 0 150
C11 b2 c2 470p
C7 b2 d2 1u
D1 d2 c2 pair
D2 c2 d2 pair
R8 c2 low 39k
C8 low 0 10n
C9 c2 high 4n
R5 high 0 22k
'''+resistance('th','high','tone',(1-tone)*1e5)+resistance('tl','tone','low',tone*1e5)+'''C3 tone b1 100n
Q1 c1 b1 e1 silicon
R7 rail b1 430k
R3 b1 0 100k
R6 rail c1 15k
R4 e1 0 3.3k
C2 c1 vt 100n
'''+resistance('vtop','vt','jack',(1-volume)*1e5)+resistance('vbot','jack','0',volume*1e5)+'''Rload jack 0 1Meg
'''+MODEL


def write(path,header,rows):
    with path.open('w',newline='') as f:
        w=csv.writer(f,lineterminator='\n');w.writerow(header)
        w.writerows([[format(x,'.12g') if isinstance(x,float) else x for x in row] for row in rows])


def generate(exe,out,fixtures):
    out.mkdir(parents=True,exist_ok=True);fixtures.mkdir(parents=True,exist_ok=True)
    (out/'spinit').write_text('* SF009 isolated pinned ngspice; no user startup\nset num_threads=1\n')
    version=subprocess.check_output([exe,'--version'],text=True).strip()
    if 'ngspice-44.2' not in version:raise ValueError('Use existing pinned ngspice44.2')
    def reference(name,net,analysis,vectors,end=None):
        rows=run_spice(exe,out,name,net,analysis,vectors)
        log=(out/(name+'.log')).read_text()
        if not rows or any(not all(math.isfinite(v) for v in r) for r in rows):raise ValueError('Empty/nonfinite reference:'+name)
        if 'aborted' in log.lower() or (end is not None and rows[-1][0]<end-1e-10):raise ValueError('Incomplete reference:'+name)
        return rows
    devices=[]
    for vc in [-.1,.1,3.,8.]:
        net=f'''Independent built-in silicon NPN current sweep
Vbase b 0 0
Vcollector c 0 {vc}
Qtest c b 0 silicon
'''+MODEL
        rows=reference('device-'+str(vc),net,'dc Vbase -0.15 0.7 0.05','v(b) v(c) i(Vbase) i(Vcollector)')
        devices.extend([r[1],r[1]-r[2],-r[3],-r[4]] for r in rows)
    write(fixtures/'device.csv',['vbe_V','vbc_V','base_A','collector_A'],devices)
    rows=reference('diodes','Independent antiparallel pair\nVtest d 0 0\nDpos d 0 pair\nDneg 0 d pair\n'+MODEL,'dc Vtest -0.7 0.7 0.05','v(d) i(Vtest)')
    write(fixtures/'diodes.csv',['v_V','net_A'],[[r[1],-r[2]] for r in rows])
    bias=[]
    for fault in [False,True]:
        for position in [0.,.5,1.]:
            net=circuit(fault,position,position,position)
            r=reference(f'bias-{int(fault)}-{position}',net,'op',' '.join('v('+n+')' for n in NODES))[0]
            bias.append([int(fault),position,position,position,*r[1:]])
    write(fixtures/'bias.csv',['fault','sustain','tone','volume',*NODES],bias)
    ac=[]
    settings=[('tone',.75,t,.5) for t in [0.,.5,1.]]+[('sustain',s,.5,.5) for s in [0.,.5,1.]]+[('volume',.75,.5,v) for v in [0.,.5,1.]]
    for kind,s,t,v in settings:
        rows=reference(f'ac-{kind}-{s}-{t}-{v}',circuit(False,s,t,v),'ac dec 12 100 10000','v(c2) v(tone) v(jack)')
        for r in rows:
            # Keep selected exact decade/nearby grid points; full AC netlist retained.
            if any(abs(math.log(r[0]/f))<1e-8 for f in [100,1000,10000]):ac.append([kind,s,t,v,*r])
    write(fixtures/'ac.csv',['control','sustain','tone','volume','Hz','c2_real','c2_imag','tone_real','tone_imag','jack_real','jack_imag'],ac)
    transients=[];envelopes=[];coarse=[]
    vectors=' '.join('v('+n+')' for n in NODES)+' v(b2,d2) v(c3,i2) v(tone) v(jack) i(Bsrc) @d3[id] @d4[id] @d1[id] @d2[id]'
    for fault in [False,True]:
        for attack in [False,True]:
            for step in ([2e-6,.5e-6] if attack else [.5e-6]):
                name=f'transient-{int(fault)}-{int(attack)}-{step}'
                # Gear2 avoids the observed near-cutoff trapezoidal startup failure.
                analysis=f'save all @d1[id] @d2[id] @d3[id] @d4[id]\noption method=gear maxord=2 reltol=1e-6 abstol=1e-11 vntol=1e-7 gmin=1e-12\ntran 5u 0.65 0 {step:.17g}\nlinearize'
                rows=reference(name,circuit(fault,attack=attack),analysis,vectors,.65)
                if len(rows)!=130001:raise ValueError('Wrong uniform reference length')
                def values(r):return [*r[1:22],-r[22],r[23]-r[24],r[25]-r[26]] # 17nodes,C7,C13,tone,jack,input,pairs.
                selected=[]
                for r in rows:
                    tick=round(r[0]*2000)
                    if abs(r[0]-tick/2000)<1e-10 and ((.02<=r[0]<=.075) or (.09<=r[0]<=.11) or (.22<=r[0]<=.275) or (.3<=r[0]<=.32) or (.6<=r[0]<=.62)):
                        selected.append([int(fault),int(attack),tick*96,r[0],*values(r)])
                if step==.5e-6:
                    transients.extend(selected)
                    for t in [.09,.13,.18,.3,.35,.45,.6]:
                        window=rows[round(t*200000):round((t+.02)*200000)]
                        # Uniform5us samples, not an unweighted RMS of adaptive steps.
                        rms=math.sqrt(sum(r[21]**2 for r in window)/len(window))
                        means=[sum(r[k] for r in window)/len(window) for k in [7,8,9,18,19]]
                        envelopes.append([int(fault),int(attack),t,t+.02,rms,*means])
                else:coarse.extend(selected)
                (out/(name+'.txt')).unlink() # raw uniform arrays are disposable, not fixture evidence.
    write(fixtures/'transient.csv',['fault','attack','sample_192k','seconds',*NODES,'C7_B2_D2_V','C13_C3_i2_V','tone_V','jack_V','input_A','pair_Q3_A','pair_Q2_A'],transients)
    write(fixtures/'recovery.csv',['fault','attack','start_s','end_s','jack_rms_V','mean_B2_V','mean_C2_V','mean_E2_V','mean_C7_V','mean_C13_V'],envelopes)
    bykey={(r[0],r[1],r[2]):r for r in transients};voltageErrors=[abs(a-b) for r in coarse for a,b in zip(r[4:25],bykey[(r[0],r[1],r[2])][4:25])];currentErrors=[abs(a-b) for r in coarse for a,b in zip(r[25:],bykey[(r[0],r[1],r[2])][25:])]
    # Retain the rejected local candidate independently, no third native voice.
    candidate=reference('candidate-r15-1M',circuit(rf=1e6),'op','v(b2) v(c2) v(e2)')[0]
    candidateWindows=[]
    for attack in [False,True]:
        analysis='option method=gear maxord=2 reltol=1e-6 abstol=1e-11 vntol=1e-7 gmin=1e-12\ntran 5u .65 0 2u\nlinearize'
        rows=reference('candidate-history-'+str(int(attack)),circuit(rf=1e6,attack=attack),analysis,'v(jack)',.65)
        candidateWindows.append([math.sqrt(sum(r[1]**2 for r in rows[round(t*200000):round((t+.02)*200000)])/4000) for t in [.09,.6]])
        (out/('candidate-history-'+str(int(attack))+'.txt')).unlink()
    write(fixtures/'candidate.csv',['R15_ohms','B2_V','C2_V','E2_V','probe_start_s','fresh_rms_V','preceded_rms_V','ratio'],[[1e6,*candidate[1:],t,candidateWindows[0][i],candidateWindows[1][i],candidateWindows[1][i]/candidateWindows[0][i]] for i,t in enumerate([.09,.6])])
    netlists=fixtures/'netlists';netlists.mkdir(exist_ok=True)
    for p in out.glob('*.cir'):shutil.copyfile(p,netlists/p.name)
    manifest=dict(simulator=version,source='ElectroSmash V3 image8b01ada; independent standard ngspice BJT/diode implementation, not native stamps',method='built-in Ebers-Moll-reduced silicon; transient Gear2, maxstep0.5us vs2us; uniform5us sampling; exact-wire pot endpoints',reference_refinement_peak_V=max(voltageErrors),reference_refinement_peak_A=max(currentErrors),startup_sha256=hashlib.sha256((out/'spinit').read_bytes()).hexdigest(),netlist_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(netlists.glob('*.cir'))},fixture_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(fixtures.glob('*.csv'))},limitations='Nominal generic silicon, not measured BC239/EHX. Intrinsic charge/Early/high injection/noise omitted; all13 external capacitors/loadings retained. Modern native thermal constants differ slightly from simulator constants; errors are measured.')
    (fixtures/'spice-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps({'device_points':len(devices),'bias':len(bias),'transient':len(transients),'reference_refinement_peak_V':max(voltageErrors),'reference_refinement_peak_A':max(currentErrors)},indent=2))

def compare_reference(actual,expected):
    a=json.loads((actual/'spice-manifest.json').read_text());b=json.loads((expected/'spice-manifest.json').read_text())
    if a['netlist_sha256']!=b['netlist_sha256'] or a['simulator']!=b['simulator']:raise ValueError('Different source netlists/simulator')
    peakV=peakA=0
    for path in sorted(expected.glob('*.csv')):
        with path.open() as f:old=list(csv.DictReader(f))
        with (actual/path.name).open() as f:new=list(csv.DictReader(f))
        if len(old)!=len(new):raise ValueError('Reference row mismatch:'+path.name)
        for left,right in zip(old,new):
            if left.keys()!=right.keys():raise ValueError('Reference fields mismatch')
            for key in left:
                try:x,y=float(left[key]),float(right[key])
                except ValueError:
                    if left[key]!=right[key]:raise ValueError('Reference label mismatch')
                    continue
                current=key.endswith('_A')
                if not math.isclose(x,y,rel_tol=2e-5 if current else 2e-6,abs_tol=1e-10 if current else 5e-7):raise ValueError('Reference numeric mismatch:'+path.name+':'+key)
                if current:peakA=max(peakA,abs(x-y))
                else:peakV=max(peakV,abs(x-y))
    result=dict(netlists_identical=True,simulator_identical=True,peak_noncurrent_numeric_error=peakV,peak_current_error_A=peakA,note='Tolerance-based reproducibility, not refreshing native/original goldens')
    (actual/'reproduction-comparison.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ngspice',required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True);p.add_argument('--check-existing',type=Path);a=p.parse_args();generate(a.ngspice,a.output,a.fixtures)
    if a.check_existing:compare_reference(a.fixtures,a.check_existing)
