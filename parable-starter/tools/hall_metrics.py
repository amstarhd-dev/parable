#!/usr/bin/env python3
"""Offline IR/scattering evidence, NumPy2.3.5; no audio synthesis/replacement."""
import argparse,json,wave
from pathlib import Path
import numpy as np
p=argparse.ArgumentParser();p.add_argument('--selected',type=Path,required=True);p.add_argument('--cloud',type=Path);p.add_argument('--verbity',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
def ir(path):
 x=np.fromfile(path,dtype='<f4').astype(float);energy=np.cumsum((x*x)[::-1])[::-1];db=10*np.log10(np.maximum(energy,1e-300)/energy[0]);i=np.flatnonzero((db<=-5)&(db>=-25));slope=np.polyfit(i/48000,db[i],1)[0]
 return x,dict(t20_extrapolated_rt60=-60/slope,first_above_minus60_seconds=float(np.flatnonzero(abs(x)>max(abs(x))*.001)[0]/48000),energy_before_80ms=float(np.sum(x[:3840]**2)/energy[0]),late_12_14s_rms=float(np.sqrt(np.mean(x[576000:672000]**2))),hf_damping_note='Source filters/averages retained; no separately calibrated air absorption model')
x,h=ir(a.selected/'healthy_ir.f32');y,f=ir(a.selected/'fault_ir.f32');_,v=ir(a.verbity/'impulse.f32')
# Independent normalized Householder construction, not copied component equations.
u=np.ones(5)/np.sqrt(5);H=np.eye(5)-2*np.outer(u,u);bounds=[]
for focus in [0,.25,.5,.75,.98,1]:
 S=(1-focus)*H+focus*np.eye(5);sigma=float(np.linalg.svd(S,compute_uv=False)[0]);assert sigma<=1+1e-14;bounds.append(dict(focusing=focus,largest_singular_value=sigma))
# Coarse predeclared50Hz bands, 1..5s window. Ratios are relative to healthy,
# not loudness matching or a selected sine artificially replacing the guitar.
N=192000;win=np.hanning(N);freq=np.fft.rfftfreq(N,1/48000)
A=abs(np.fft.rfft(x[48000:240000]*win))**2;B=abs(np.fft.rfft(y[48000:240000]*win))**2;bands=[]
for center in range(150,3001,50):
 k=(freq>=center-25)&(freq<center+25);bands.append(dict(center_hz=center,fault_over_healthy_db=float(10*np.log10((B[k].sum()+1e-25)/(A[k].sum()+1e-25)))))
def pcm(path):
 with wave.open(str(path),'rb') as w:return np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').reshape(-1,w.getnchannels()).astype(float)/32768
stereo=pcm(a.selected/'healthy_stereo.wav');mono=np.mean(stereo,axis=1);assert np.sqrt(np.mean(mono*mono))>.005
weakH=pcm(a.selected/'weak_healthy.wav').ravel();weakF=pcm(a.selected/'weak_fault.wav').ravel();chordH=pcm(a.selected/'chord_healthy.wav').ravel();chordF=pcm(a.selected/'chord_fault.wav').ravel()
def diff(aa,bb):return dict(difference_rms=float(np.sqrt(np.mean((aa-bb)**2))),healthy_rms=float(np.sqrt(np.mean(aa*aa))),fault_rms=float(np.sqrt(np.mean(bb*bb))))
report=dict(numpy=np.__version__,rate=48000,healthy=h,fault=f,verbity=v,scattering_independent_svd=bounds,late_loop_retention_upper_before_filters=3125*.000293,healthy_stereo_mix_correlation=float(np.corrcoef(stereo.T)[0,1]),healthy_mono_fold_rms=float(np.sqrt(np.mean(mono*mono))),fault_bands_1_5s=bands,weak_pcm=diff(weakH,weakF),chord_pcm=diff(chordH,chordF),limitations=['T20 is an extrapolated composite response, not a measured acoustic room RT60.','Scattering contraction alone does not prove complete nonlinear engine stability; native sustained/overload/decay tests are separate.','Focusing is substantially linear; normalized weak/fault difference is not expected to vanish.','Audibility/expensive hall quality require independent listening; no physical hall/480L calibration.'])
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='fault_bands_1_5s'},indent=2))
