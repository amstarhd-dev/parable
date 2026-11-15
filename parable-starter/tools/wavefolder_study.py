#!/usr/bin/env python3
"""Reproduce alternative selection using native captures and independent SPICE.

Requires offline NumPy2.3.5. Run wavefolder_study.cpp first and the independent
generator into --spice-work. Retains complete matrix, never changes gate values.
"""
import argparse,json,math
from pathlib import Path
import numpy as np
from wavefolder_spice_reference import reference_wave,fft_up,SIZE
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--captures',required=True,type=Path);p.add_argument('--spice-work',required=True,type=Path);a=p.parse_args();results=[];tables=[]
dense={rp:np.loadtxt(a.spice_work/f'dense-RP{rp}.txt',skiprows=1) for rp in [15000,22000]}
knots=np.genfromtxt(a.captures/'knots.csv',delimiter=',',names=True)
for rp in [15000,22000]:
    for count in [256,512,1024]:
        k=knots[(knots['Rplus']==rp)&(knots['intervals']==count)];x=dense[rp][:,0];q=np.clip((x+3)/(6/count),0,count);i=np.minimum(q.astype(int),count-1);t=q-i;h=6/count
        v=k['readout_V'];s=k['derivative'];c0=v[i];c1=h*s[i];c2=3*(v[i+1]-c0)-2*c1-h*s[i+1];c3=2*(c0-v[i+1])+c1+h*s[i+1];y=c0+t*(c1+t*(c2+t*c3));error=y+dense[rp][:,1]
        tables.append(dict(Rplus=rp,intervals=count,lookup_bytes=16*(count+1),peak_V=float(max(abs(error))),RMS_V=float(np.sqrt(np.mean(error**2)))))
def Wlog(L):
    w=math.exp(L) if L<-8 else max(.1,L-math.log(L) if L>1 else math.log1p(math.exp(L)))
    for _ in range(16):w=max(w*.5,w-(w+math.log(w)-L)/(1+1/w))
    return w
x=dense[15000][:,0][::20];paper=np.array([2*10000/15000*u-np.sign(u)*.025864*Wlog(math.log(10000*1e-17/.025864)+(1+2*10000/15000)*abs(u)/.025864) for u in x]);paper_error=paper+dense[15000][::20,1]
for rate in [44100,48000,96000]:
    for fault,rp in [(0,15000),(1,22000)]:
        for kind in range(4):
            for factor in [1,2,4]:
                raw=np.loadtxt(a.captures/f'capture-{rate}-{fault}-{kind}-{factor}-0.csv',delimiter=',',skiprows=1);x=raw[:,0]
                if factor==1:
                    signal=np.interp(fft_up(x,SIZE*128)*6,dense[rp][:,0],-dense[rp][:,1]);spectrum=np.fft.rfft(signal)/(SIZE*128);omega=2*np.pi*np.arange(SIZE//2+1)/SIZE;g0=math.tan(math.pi*(1/(2*math.pi*.01))/rate);g=g0/(1+g0);z=np.exp(-1j*omega);target=spectrum[:SIZE//2+1]*(1-g)*(1-z)/(1-(1-2*g)*z)/8
                else:_,target=reference_wave(x,rate,dense[rp],factor=factor)
                power=2*sum(abs(target[1:-1])**2);omega=2*np.pi*np.arange(len(target))/SIZE;bin_=int(np.argmax(abs(np.fft.rfft(x))[1:]))+1
                for aa in [0,1]:
                    capture=np.loadtxt(a.captures/f'capture-{rate}-{fault}-{kind}-{factor}-{aa}.csv',delimiter=',',skiprows=1);actual=np.fft.rfft(capture[:,1])/SIZE*np.exp(1j*omega*.5/factor*aa);error=math.sqrt(2*sum(abs(actual[1:-1]-target[1:-1])**2));mask=np.ones(len(actual),dtype=bool);mask[0]=False
                    if kind<3:mask[np.arange(bin_,len(mask),bin_)]=False
                    spur=20*math.log10(max(2*max(abs(actual[mask]))/math.sqrt(power),1e-30)) if kind<3 else None
                    results.append(dict(rate=rate,fault=fault,kind=kind,factor=factor,ADAA=aa,actual_Hz=bin_*rate/SIZE if kind<3 else None,relative_RMS=error/math.sqrt(power),spur_dBr_reference_RMS=spur))
report=dict(native_output_divisor=8,paper_symmetric_Lambert_W_peak_V=float(max(abs(paper_error))),paper_note='Different ideal symmetry/finite-beta approximations; not a paper error. No asymmetric post-shift allowed.',tables=tables,sampling=results)
(a.captures/'selection.json').write_text(json.dumps(report,indent=2)+'\n');print('Reproduced6 lookup candidates and144 full-processor sampling cases')
