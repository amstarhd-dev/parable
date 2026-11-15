#!/usr/bin/env python3
"""SF015 offline measurements/figures from actual native audio and SPICE fixtures."""
import argparse,csv,json,wave
from pathlib import Path
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--renders',type=Path,required=True);p.add_argument('--study',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);data=a.output/'data';figs=a.output/'figures';data.mkdir(exist_ok=True);figs.mkdir(exist_ok=True)
fixtures=root/'tests/fixtures/fz1'
def rows(p):return list(csv.DictReader(p.open()))
def wav(name):
 with wave.open(str(a.renders/name),'rb') as w:return w.getframerate(),np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').astype(float)/32768
pairs=[]
for label in ['attack_low','attack_mid','attack_high','input_low','input_mid','input_high']:
 rate,h=wav(label+'_healthy_raw.wav');_,f=wav(label+'_feed_open_raw.wav');scale=float(h@f/(h@h));residual=float(np.sqrt(np.mean((f-scale*h)**2))/np.sqrt(np.mean(f*f)))
 pairs.append(dict(group=label,fault_over_healthy_RMS_dB=float(20*np.log10(np.linalg.norm(f)/np.linalg.norm(h))),best_fader_fit=scale,remaining_relative_RMS=residual,correlation=float(np.corrcoef(h,f)[0,1])))
with(data/'fault-shape.csv').open('w',newline='') as f:w=csv.DictWriter(f,fieldnames=list(pairs[0]));w.writeheader();w.writerows(pairs)
# Exact carrier fits describe independent dynamic reference, not a static curve.
harmonics=[]
for fault in [0,1]:
 for level in [.02,.1,.2]:
  name=f'equations-sine-{fault}-{level}.csv';r=rows(fixtures/name);t=np.array([float(x['seconds']) for x in r]);y=np.array([float(x['out_V']) for x in r]);keep=t>=.07;t=t[keep];y=y[keep];basis=[np.ones_like(t),t-np.mean(t)]
  for h in range(1,10):basis.extend([np.sin(2*np.pi*196*h*t),np.cos(2*np.pi*196*h*t)])
  c=np.linalg.lstsq(np.asarray(basis).T,y,rcond=None)[0];v=[float(np.hypot(c[2*h],c[2*h+1])) for h in range(1,10)];harmonics.append(dict(fault=fault,input_peak_V=level,H1_V=v[0],H2_over_H1_dB=float(20*np.log10(v[1]/v[0])),H3_over_H1_dB=float(20*np.log10(v[2]/v[0])),H5_over_H1_dB=float(20*np.log10(v[4]/v[0])),window_start_s=.07,window_end_s=.12))
with(data/'reference-harmonics.csv').open('w',newline='') as f:w=csv.DictWriter(f,fieldnames=list(harmonics[0]));w.writeheader();w.writerows(harmonics)
# Decay is quantified as real output/input gain, no arbitrary silence threshold.
decays=[]
fig,ax=plt.subplots(2,2,figsize=(12,8),constrained_layout=True)
for fault,color in [(0,'#16708d'),(1,'#bc5a28')]:
 r=rows(fixtures/f'equations-pluck-{fault}-0.5.csv');t=np.array([float(x['seconds']) for x in r]);x=np.array([float(x['input_V']) for x in r]);y=np.array([float(x['out_V']) for x in r]);b3=np.array([float(x['b3']) for x in r]);c3=np.array([float(x['c3']) for x in r]);name='healthy' if not fault else 'Q2 feed open';mid=[];rr=[];ii=[]
 for start in np.arange(.02,.83,.02):
  k=(t>=start)&(t<start+.02);xr=float(np.sqrt(np.mean(x[k]**2)));yr=float(np.sqrt(np.mean(y[k]**2)));mid.append(start+.01);rr.append(yr);ii.append(xr);decays.append(dict(fault=fault,t_s=start+.01,input_RMS_V=xr,out_RMS_V=yr,gain=yr/xr))
 ax[0,0].semilogy(mid,rr,label=name,color=color);ax[0,1].plot(t,c3,label=name,color=color);k=(t>=.03)&(t<.08);ax[1,0].plot(t[k]*1000,y[k],label=name,color=color,marker='.',markersize=2)
 # Actual delivered PCM after normal FIR,4V divisor restored; shift known48frame latency.
 rate,native=wav('pluck_decay_'+('healthy' if not fault else 'feed_open')+'_raw.wav');tn=np.arange(len(native))/rate-48/rate;kn=(tn>=.03)&(tn<.08);ax[1,1].plot(tn[kn]*1000,native[kn]*4,label=name,color=color)
ax[0,0].semilogy(mid,ii,'k--',label='input');ax[0,0].set(title='Full SPICE pluck: finite tail, no hard gate',xlabel='seconds',ylabel='20ms RMS volts');ax[0,1].set(title='Q3 collector recovery',xlabel='seconds',ylabel='physical volts');ax[1,0].set(title='Independent full-GP SPICE output',xlabel='milliseconds',ylabel='jack volts');ax[1,1].set(title='Delivered native4x output (known latency aligned)',xlabel='milliseconds',ylabel='jack volts after FIR')
for q in ax.flat:q.grid(alpha=.2);q.legend(fontsize=8)
fig.savefig(figs/'decay.png',dpi=140);plt.close(fig)
with(data/'decay.csv').open('w',newline='') as f:w=csv.DictWriter(f,fieldnames=list(decays[0]));w.writeheader();w.writerows(decays)
# Bias and documented sensitivity; source/load effects separately compared in tests.
fig,ax=plt.subplots(2,2,figsize=(12,8),constrained_layout=True);dc=rows(fixtures/'equations-dc.csv');temp=rows(fixtures/'temperature.csv')
for fault,color in [(0,'#16708d'),(1,'#bc5a28')]:
 name='healthy' if not fault else 'Q2 feed open';r=[v for v in dc if int(v['fault'])==fault and float(v['supply_V'])==3 and float(v['leakage_scale'])==1];attack=[float(v['attack']) for v in r];ax[0,0].plot(attack,[float(v['c2']) for v in r],'o-',label=name,color=color);ax[0,1].plot(attack,[-1e6*float(v['q2_ic_A']) for v in r],'o-',label=name,color=color)
 r=[v for v in temp if int(v['fault'])==fault];ax[1,0].plot([float(v['temp_C']) for v in r],[float(v['c2']) for v in r],'o-',label=name,color=color);ax[1,1].plot([float(v['temp_C']) for v in r],[float(v['c3']) for v in r],'o-',label=name,color=color)
ax[0,0].set(title='Q2 loaded DC collector',xlabel='Attack electrical fraction',ylabel='volts');ax[0,1].set(title='Actual Q2 collector current magnitude',xlabel='Attack electrical fraction',ylabel='microamps');ax[1,0].set(title='Offline SPICE temperature sensitivity: Q2',xlabel='degrees C',ylabel='volts');ax[1,1].set(title='Offline SPICE temperature sensitivity: Q3',xlabel='degrees C',ylabel='volts')
for q in ax.flat:q.grid(alpha=.2);q.legend(fontsize=8)
fig.savefig(figs/'bias.png',dpi=140);plt.close(fig)
fig,ax=plt.subplots(2,1,figsize=(12,7),constrained_layout=True)
for name,color in [('dry','#555555'),('healthy','#16708d'),('feed_open','#bc5a28')]:
 rate,y=wav('attack_mid_'+name+'_rms_matched.wav');n=np.arange(len(y));keep=(n/rate>=2)&(n/rate<2.08);ax[0].plot(n[keep]/rate,y[keep],label=name,color=color,alpha=.8)
 start=2*rate;part=y[start:start+8192]*np.hanning(8192);s=abs(np.fft.rfft(part));s/=max(s.max(),1e-30);ax[1].plot(np.fft.rfftfreq(8192,1/rate),20*np.log10(np.maximum(s,1e-8)),label=name,color=color)
ax[0].set(title='Actual take03, Attack0.5, matched levels',xlabel='seconds',ylabel='normalized PCM');ax[1].set(title='Same take segment, spectra normalized to each peak (shape only)',xlabel='Hz',ylabel='dB relative own spectrum peak',xlim=(0,20000),ylim=(-100,2))
for q in ax:q.grid(alpha=.2);q.legend(fontsize=8)
fig.savefig(figs/'guitar.png',dpi=140);plt.close(fig)
summary=dict(fault_shape=pairs,independent_reference_harmonics=harmonics,decay_tail=[v for v in decays if v['t_s']>.8],listening_status='Generated actual native WAVs; no human/founder approval or hard-gating claim',full_band_sampling_gate_misses_4x=sum(r['factor']=='4' and r['gate_pass']=='False' for r in rows(a.study/'alias.csv')),hardware_runtime='not measured',plot_note='PCM quantization zeros are not a physical gate; synthetic transient versus native normal FIR is labeled, not an assertion of sample identity.')
(data/'summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary,indent=2))
