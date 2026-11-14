#!/usr/bin/env python3
"""Optional offline scientific figures; requires Matplotlib, never a DSP dependency."""
import argparse,csv,math
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixtures',type=Path,required=True);p.add_argument('--audio',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
plt.rcParams.update({'svg.hashsalt':'parable-sf011','font.size':9,'axes.grid':True,'grid.alpha':.25})
def rows(path):
 with path.open() as f:return list(csv.DictReader(f))
def save(fig,name):
 fig.savefig(a.output/name,metadata={'Date':None,'Creator':'Parable SF-011 / Matplotlib '+matplotlib.__version__});plt.close(fig)
 path=a.output/name;path.write_text('\n'.join(line.rstrip() for line in path.read_text().splitlines())+'\n')
ac=rows(a.fixtures/'ac.csv');fig,ax=plt.subplots(2,2,figsize=(11,7),layout='constrained')
for gain,color in [(0,'#287b42'),(.5,'#276dad'),(.95,'#a14791')]:
 for fault in [0,1]:
  data=[r for r in ac if float(r['gain'])==gain and float(r['treble'])==.5 and float(r['level'])==.5 and int(r['fault'])==fault];x=[float(r['hz']) for r in data];v=np.array([complex(float(r['out_real']),float(r['out_imag'])) for r in data]);style='--' if fault else '-';label=f'Gain{gain:g} '+('contact100k' if fault else 'healthy')
  ax[0,0].semilogx(x,20*np.log10(np.maximum(abs(v),1e-30)),style,color=color,label=label);ax[1,0].semilogx(x,np.unwrap(np.angle(v))*180/math.pi,style,color=color)
for name,color in [('drive_A','#ae433c'),('clean1_A','#287b42'),('clean2_A','#276dad')]:
 for fault in [0,1]:
  data=[r for r in ac if float(r['gain'])==.5 and float(r['treble'])==.5 and float(r['level'])==.5 and int(r['fault'])==fault];x=[float(r['hz']) for r in data];v=np.array([complex(float(r[name+'_real']),float(r[name+'_imag'])) for r in data]);style='--' if fault else '-'
  ax[0,1].semilogx(x,abs(v)*1e6,style,color=color,label=name+' '+('100k' if fault else 'healthy'));ax[1,1].semilogx(x,np.unwrap(np.angle(v))*180/math.pi,style,color=color)
ax[0,0].set(title='Independent SPICE: output magnitude',ylabel='dB V/V');ax[1,0].set(ylabel='Output phase (degrees)',xlabel='Hz');ax[0,1].set(title='Gain.5: actual loaded current groups',ylabel='microamp/V');ax[1,1].set(ylabel='Current phase (degrees)',xlabel='Hz');ax[0,0].legend(fontsize=7);ax[0,1].legend(fontsize=7)
fig.suptitle('SF-011: nominal S698 / one P1A contact (no physical calibration)\nSmall signal, Treble.5 / Output.5; current groups are not isolated audio sources')
save(fig,'klon-frequency-phase.svg')
polarity=rows(a.audio/'polarity.csv');fig,ax=plt.subplots(3,1,figsize=(11,8),layout='constrained')
for fault,color in [(0,'#287b42'),(1,'#ce6332')]:
 data=[r for r in polarity if int(r['fault'])==fault];x=np.array([float(r['last_phase11_s']) for r in data])*1000;label='contact100k' if fault else 'healthy'
 ax[0].plot(x,[float(r['clip_V']) for r in data],color=color,label=label)
 ax[1].plot(x,[float(r['sum_out_V'])-float(r['sum_minus_V']) for r in data],color=color,label=label)
 for key,style in [('drive_feedback_V','-'),('clean1_feedback_V','--'),('clean2_feedback_V',':')]:ax[2].plot(x,[float(r[key]) for r in data],style,color=color,label=label+' '+key)
ax[0].set(title='Same native C++ / balanced.2V 1kHz input',ylabel='Shunt clip V');ax[1].set(ylabel='SumOut - SumMinus V');ax[2].set(ylabel='Feedback contribution V',xlabel='Last internalphase11 time (ms)');ax[0].legend();ax[2].legend(ncol=2,fontsize=7)
fig.suptitle('SF-011: actual loaded paths, before Tone and output FIR\nContributions obey KCL; this is not a naive sum of final-output audio taps')
save(fig,'klon-loaded-waveforms.svg')
supply=rows(a.fixtures/'supply-island.csv');fig,ax=plt.subplots(1,2,figsize=(11,4),layout='constrained')
base=[r for r in supply if float(r['pump_hz'])==17500 and float(r['switch_ohms'])==23 and float(r['cap_ESR_ohms'])==1]
x=np.array([float(r['load_A']) for r in base])*1000
for k,key,ripple,nominal in [(0,'mean_positive_V','positive_ripple_pp_V',16.2),(1,'mean_negative_V','negative_ripple_pp_V',-8.6)]:
 ax[k].errorbar(x,[float(r[key]) for r in base],yerr=np.array([float(r[ripple]) for r in base])/2,fmt='o-',capsize=4,label='Illustrative SPICE: mean +/- half ripple')
 ax[k].axhline(nominal,color='#b95531',ls='--',label='Fixed nominal audio-model rail');ax[k].set(xlabel='Assumed pair quiescent load (mA)',ylabel='V');ax[k].legend(fontsize=7)
fig.suptitle('Original inverter+doubler topology, illustrative switches/ESR/load\n17.5kHz /23ohm /1ohm ESR; NOT measured vintage9V supply or PSRR equivalence')
save(fig,'klon-supply-domain.svg')
(a.output/'PLOT_ENVIRONMENT.txt').write_text('Optional offline plots: Matplotlib '+matplotlib.__version__+'; NumPy '+np.__version__+'\n')
