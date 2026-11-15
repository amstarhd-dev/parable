#!/usr/bin/env python3
"""Offline research plots, NumPy2.3.5 and Matplotlib3.10.8; no runtime/build dependency."""
from pathlib import Path
import csv, numpy as np, matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
p=Path(__file__).resolve().parents[1];r=p/'tests/fixtures/wavefolder';out=p/'docs/figures/sf014';out.mkdir(parents=True,exist_ok=True)
a=np.genfromtxt(r/'dc.csv',delimiter=',',names=True);f,axes=plt.subplots(2,2,figsize=(10,7))
for rp,label in [(15000,'Healthy15k/15k'),(22000,'Only upper feed22k'),(33000,'33k rejected')]:
 d=a[a['Rplus_ohms']==rp];axes[0,0].plot(d['input_V'],-d['collector_V'],label=label)
 if rp!=33000:
  axes[0,1].plot(d['input_V'],np.gradient(-d['collector_V'],d['input_V']),label=label)
  axes[1,0].plot(d['input_V'],d['QP_IC_A']*1000,label=label+' PNP');axes[1,0].plot(d['input_V'],d['QN_IC_A']*1000,ls='--',label=label+' NPN')
axes[0,0].set(xlabel='Buffered input/model V',ylabel='Raw inverted collector/model V');axes[0,1].set(xlabel='Input/model V',ylabel='Transfer derivative');axes[0,1].axhline(0,color='k',lw=.5);axes[1,0].set(xlabel='Input/model V',ylabel='Actual collector current/mA')
h=np.genfromtxt(r/'harmonics.csv',delimiter=',',names=True)
for rp,label in [(15000,'Healthy'),(22000,'Upper22k')]:
 d=h[(h['Rplus_ohms']==rp)&(h['input_bias_V']==0)];axes[1,1].plot(d['input_peak_V'],d['H2_V'],marker='o',label=label+' H2');axes[1,1].plot(d['input_peak_V'],d['H3_V'],marker='o',label=label+' H3')
axes[1,1].set(xlabel='Sine input peak/model V',ylabel='Static-law harmonic peak/model V')
for ax in axes.flat:ax.grid(alpha=.25);ax.legend(fontsize=7)
f.suptitle('SF014 independent builtin-BJT ngspice44.2: one loaded Lockhart, no native curve');f.tight_layout();f.savefig(out/'circuit.png',dpi=125);plt.close(f)
# Resampler L1 containment proof for arbitrary accepted sample histories.
from sys import path
path.insert(0,str(p/'tools'));from wavefolder_spice_reference import fir
bounds={}
for factor in [1,2,4]:
 h=fir(factor).astype(np.float32) if factor>1 else np.ones(1)
 bounds[factor]=float(max(sum(abs(h[k::factor])) for k in range(factor))*factor*.25*6)
assert max(bounds.values())<3
print('All-history FIR L1 input bound at Drive6:',bounds)
