#!/usr/bin/env python3
"""Independent complex transfer solution of the complete static delay network.
No native C++ calls, impulse recurrence or borrowed implementation here.
"""
import argparse,json,math,wave
from pathlib import Path
import numpy as np
p=argparse.ArgumentParser();p.add_argument('--directory',type=Path,required=True);a=p.parse_args();root=a.directory;fs=48000
r=np.float32(fs/29761);ratio=np.float32(.3)/np.float32(2)
times=[float(np.float32(.004)*np.float32(fs))]+[float(np.ceil(np.float32(n)*r)) for n in [142,107,379,277]]+[float(np.float32(np.ceil(np.float32(2*n)*r))*ratio) for n in [672,4453,1800,3720,908,4217,2656,3163]]
left=np.array([266,2974,1913,1996,1990,187,1066],dtype=np.float32)*r*ratio
right=np.array([353,3627,1228,2673,2111,335,121],dtype=np.float32)*r*ratio
gain=float(np.float32(math.exp(math.log(.001)*(.3*21589/29761)/(4*.62))))
# Evaluate the independently assembled frequency-domain network at actual FFT bins.
n=6*fs;k=np.unique(np.geomspace(2,n//2-1,1200).astype(int));z=np.exp(-2j*np.pi*k/n)
def delay(t,before_write=True):
 whole=math.floor(float(t));frac=float(t)-whole
 return z**(whole+int(before_write))*((1-frac)+frac*z)
A=[delay(t) for t in times]
def ap(i,g):return (A[i]-g)/(1-g*A[i])
def lp(hz):
 b=float(np.float32(math.exp(-2*math.pi*hz/fs)));aa=float(np.float32(1-np.float32(b)))
 return aa/(1-b*z)
S=A[0]*lp(10000)*ap(1,.75)*ap(2,.75)*ap(3,.625)*ap(4,.625)
reports={};impulses={};curves={}
for name,g2 in [('healthy',.5),('fault',float(np.float32(.94)))]:
 TL=ap(5,-.7)*A[6]*lp(6500)*gain*ap(7,.5)*A[8]
 TR=ap(9,-.7)*A[10]*lp(6500)*gain*ap(11,g2)*A[12]
 den=1-gain*gain*z*z*TL*TR
 Lin=S*(1+gain*z*TR)/den;Rin=S*(1+gain*z*TL)/den
 # Histories observed by original output taps after current sample write.
 L1=ap(5,-.7)*Lin;R1=ap(9,-.7)*Rin
 L2=L1*A[6]*lp(6500)*gain/(1-.5*A[7]);R2=R1*A[10]*lp(6500)*gain/(1-g2*A[11])
 LD=L1*A[6]*lp(6500)*gain*ap(7,.5);RD=R1*A[10]*lp(6500)*gain*ap(11,g2)
 tap=lambda j,t:delay(t,False)*j
 outL=tap(R1,left[0])+tap(R1,left[1])-tap(R2,left[2])+tap(RD,left[3])-tap(L1,left[4])-tap(L2,left[5])-tap(LD,left[6])
 outR=tap(L1,right[0])+tap(L1,right[1])-tap(L2,right[2])+tap(LD,right[3])-tap(R1,right[4])-tap(R2,right[5])-tap(RD,right[6])
 expected=.25*(outL+outR);x=np.fromfile(root/('impulse_'+name+'.f32'),'<f4').astype(float);assert len(x)==n
 observed=np.fft.rfft(x)[k]/float(np.float32(.2));err=abs(observed-expected);relative=float(np.linalg.norm(err)/np.linalg.norm(expected))
 assert max(err)<.0005 and relative<.00005,(name,max(err),relative)
 energy=np.cumsum((x*x)[::-1])[::-1];db=10*np.log10(np.maximum(energy,1e-60)/energy[0]);mask=(db<=-5)&(db>=-25);fit=np.polyfit(np.flatnonzero(mask)/fs,db[mask],1)
 reports[name]=dict(max_complex_error=float(max(err)),relative_complex_error=relative,t20_extrapolated_rt60_s=float(-60/fit[0]),last_second_rms=float(np.sqrt(np.mean(x[-fs:]**2))),energy_300_1000ms=float(np.sum(x[int(.3*fs):fs]**2)),first_nonzero_sample=int(np.flatnonzero(x)[0]))
 impulses[name]=x;curves[name]=observed
freq=k*fs/n;ratioDb=20*np.log10(np.maximum(abs(curves['fault']),1e-20)/np.maximum(abs(curves['healthy']),1e-20));valid=(freq>=200)&(freq<=8000)&(abs(curves['healthy'])>.05)
idx=np.flatnonzero(valid)[np.argmax(ratioDb[valid])]
# Normalized early echo density (samples above local RMS, Gaussian expectation).
density={}
for name,x in impulses.items():
 density[name]={}
 for start,end in [(0,.01),(.01,.02),(.02,.04),(.04,.08),(.08,.16)]:
  w=x[int(start*fs):int(end*fs)];rms=np.sqrt(np.mean(w*w));density[name][str((start,end))]=float(np.mean(abs(w)>rms)/.3173105) if rms else 0
report=dict(method='independent full-network z-transform; fractional delay interpolation and existing internal tap histories; native f32 FFT compared on a logarithmic frequency grid',sample_rate=fs,frequency_bins=len(k),loop_gain=gain,one_fault='right second tank allpass feedback .5 to.94; tied feedforward coefficient uses same parameter',measurements=reports,normalized_early_echo_density=density,largest_sampled_modal_increase=dict(frequency_hz=float(freq[idx]),fault_over_healthy_db=float(ratioDb[idx])),tail_energy_300_1000ms_increase_db=float(10*np.log10(reports['fault']['energy_300_1000ms']/reports['healthy']['energy_300_1000ms'])),stability='Every interpolated delay is contractive; each allpass is a disk-preserving transfer with |g|<1; unchanged cross-loop gains<1. This is bounded linear algorithmic ringing, not amplitude-triggered behavior.',limits=['Six-second reference response truncation and native float coefficient quantization contribute residual error.','Temporal density/decay/modal differences do not establish PCM70 authenticity or musical quality.'])
(root/'reference.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
