#!/usr/bin/env python3
"""Offline NumPy alias study. Ideal bandlimit of16x exact native GP, no fitted ref."""
import argparse,csv,hashlib,json
from pathlib import Path
import numpy as np
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--study',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True);p.add_argument('--require-gates',action='store_true',help='Exit1 if any normal8x acceptance gate misses');a=p.parse_args();a.fixtures.mkdir(parents=True,exist_ok=True)
n=4096;metrics=[];reference=[]
for rate in [44100,48000,96000]:
 for fault in [0,1]:
  for kind in range(4):
   load=lambda f:np.loadtxt(a.study/f'capture-{rate}-{fault}-{kind}-{f}.csv',skiprows=1)
   high=load(0);assert len(high)==n*16 and np.isfinite(high).all()
   spectrum=np.fft.fft(high)/16;r=np.zeros(n,complex);r[:n//2]=spectrum[:n//2];r[n//2+1:]=spectrum[-n//2+1:];r[n//2]=(spectrum[n//2]+spectrum[-n//2]).real
   expected=np.fft.ifft(r).real;energy=float(np.sum(abs(r)**2));rms=float(np.sqrt(np.mean(expected**2)))
   reference.extend([rate,fault,kind,i,float(v)] for i,v in enumerate(expected))
   for factor in [1,2,4,8]:
    y=load(factor);assert len(y)==n and np.isfinite(y).all();s=np.fft.fft(y);delay=0 if factor==1 else 48;aligned=s*np.exp(2j*np.pi*np.fft.fftfreq(n)*delay);relative=float(np.sqrt(np.sum(abs(aligned-r)**2)/energy));peak=float(2*np.max(abs(aligned-r))/n/rms)
    spur=0
    if kind<3:
     bin=[1760,3520,8000][kind]*n//rate;indices=np.arange(1,n//2);spur=float(2*np.max(abs(s[indices[indices%bin!=0]]))/n/rms)
    db=float(20*np.log10(max(spur,1e-30)));gate=relative<=(.1 if kind==2 else .05) and (kind==3 or db< -45)
    # Explanatory only: remove the DECLARED output FIR response from the
    # discrepancy. This does not replace or relax the original full-band gate.
    filtered_relative=relative
    if factor>1:
     taps=48*factor+1;k=np.arange(taps);z=k-(taps-1)/2;fc=.4/factor
     h=2*fc*np.sinc(2*fc*z)*(.42-.5*np.cos(2*np.pi*k/(taps-1))+.08*np.cos(4*np.pi*k/(taps-1)));h/=np.sum(h)
     hf=abs(np.fft.fft(h,n*factor));response=np.concatenate((hf[:n//2+1],hf[-n//2+1:]));filtered=r*response
     filtered_relative=float(np.sqrt(np.sum(abs(aligned-filtered)**2)/np.sum(abs(filtered)**2)))
    metrics.append([rate,fault,kind,factor,relative,peak,db,gate,filtered_relative])
with(a.fixtures/'alias-reference.csv').open('w',newline='') as f:w=csv.writer(f,lineterminator='\n');w.writerow(['rate','fault','kind','sample','native_reference_normalized']);w.writerows(reference)
with(a.study/'alias.csv').open('w',newline='') as f:w=csv.writer(f,lineterminator='\n');w.writerow(['rate','fault','kind','factor','relative_RMS','peak_error_component_dBrms_linear','false_spur_dBrms','gate_pass','relative_after_known_output_FIR_diagnostic']);w.writerows(metrics)
receipt=dict(numpy=np.__version__,reference='16x exact native, independently ideal-bandlimited, unfiltered continuous analytic input; SPICE separately validates physics. Native1x has0 latency,2x/4x48 frames. No alignment fit. Not an independent physical transistor.',capture_sha256={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(a.study.glob('capture*.csv'))},reference_sha256=hashlib.sha256((a.fixtures/'alias-reference.csv').read_bytes()).hexdigest(),gate_misses=[r for r in metrics if not r[7]])
(a.fixtures/'alias-manifest.json').write_text(json.dumps(receipt,indent=2)+'\n')
for factor in [1,2,4,8]:
 rows=[r for r in metrics if r[3]==factor];print('factor',factor,'pass',sum(r[7] for r in rows),'of',len(rows),'max_relative',max(r[4] for r in rows),'worst_spur_dB',max(r[6] for r in rows if r[2]<3))
print('8x misses',[r for r in metrics if r[3]==8 and not r[7]])

if a.require_gates and any(r[3]==8 and not r[7] for r in metrics):raise SystemExit(1)
