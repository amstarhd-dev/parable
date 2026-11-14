#!/usr/bin/env python3
"""Optional standalone publication figures from actual SF012 receipts, no DSP.
Requires matplotlib only for plotting; host tests/research comparison are stdlib.
"""
import argparse,csv,math
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
def rows(p):return list(csv.DictReader(p.open()))
def values(rs,key):return [float(r[key]) for r in rs]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--measure',type=Path,required=True);p.add_argument('--numerical',type=Path,required=True);p.add_argument('--comparison',type=Path,required=True);p.add_argument('--audio',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    plt.rcParams.update({'font.size':9,'svg.hashsalt':'sf012','axes.grid':True,'grid.alpha':.25})
    fig,ax=plt.subplots(2,3,figsize=(14,8),layout='constrained');loop=rows(a.numerical/'loops.csv');rem=.94832898;muMs=.145/rem
    h=values(loop,'H_A_per_m');m=values(loop,'M_condensed');ax[0,0].plot([x/1000 for x in h],[4*math.pi*1e-7*x+muMs*y for x,y in zip(h,m)],lw=1);ax[0,0].set(xlabel='H (kA/m)',ylabel='B = μ₀H + μ₀Ms·m (T)',title='Positive relay population: major + minor loops')
    candidates=rows(a.fixtures/'candidates.csv')
    for bias,label in [('1','nominal12mA'),('0.5','selected6mA'),('0.25','rejected3mA')]:
        r=[r for r in candidates if float(r['bias_ratio'])==float(bias) and r['phases']=='64'];ax[0,1].plot(values(r,'input_V'),values(r,'remanence'),'o-',label=label)
    ax[0,1].set(xlabel='DC recorded driver (V)',ylabel='remanent m = M/Ms',title='Independent resolved field; identical material');ax[0,1].legend()
    tones=rows(a.measure/'tones.csv');h3=next(k for k in tones[0] if k.startswith('H3'))
    for weak,label in [('0','nominal'),('1','weak bias')]:
        r=[r for r in tones if r['rate_hz']=='48000' and r['hz']=='1000' and r['weak']==weak];ax[0,2].loglog(values(r,'input_peak_V'),[float(v['output_peak_fundamental_V'])/float(v['input_peak_V']) for v in r],'o-',label=label);ax[1,0].loglog(values(r,'input_peak_V'),[100*float(v[h3])/float(v['output_peak_fundamental_V']) for v in r],'o-',label=label)
        r=[r for r in tones if r['rate_hz']=='48000' and float(r['input_peak_V'])==.005 and r['weak']==weak];ref=float(next(x for x in r if x['hz']=='100')['output_peak_fundamental_V']);ax[1,1].semilogx(values(r,'hz'),[20*math.log10(float(v['output_peak_fundamental_V'])/ref) for v in r],'o-',label=label)
    ax[0,2].set(xlabel='input peak (V)',ylabel='fundamental output/input (V/V)',title='Actual full native path: compression');ax[0,2].legend()
    ax[1,0].set(xlabel='input peak (V)',ylabel='H3/H1 (%)',title='Actual1kHz,48kHz native harmonics');ax[1,0].legend()
    ax[1,1].set(xlabel='Hz',ylabel='dB relative100Hz',title='Small-signal record + material + pickup + EQ');ax[1,1].legend()
    pickup=rows(a.numerical/'playback.csv')
    for rate in ['44100','48000','96000']:
        r=[r for r in pickup if r['rate_hz']==rate];ax[1,2].semilogx(values(r,'hz'),values(r,'error_dB'),'o-',label=rate+'Hz')
    ax[1,2].axhline(.5,color='grey',ls=':');ax[1,2].axhline(-.5,color='grey',ls=':');ax[1,2].set(xlabel='Hz',ylabel='dB error',title='Physical pickup FIR vs analytic gap/spacing/thickness');ax[1,2].legend()
    fig.suptitle('SF012 — generic moving magnetic tape, NOT measured deck data');fig.savefig(a.output/'material-and-response.svg',metadata={'Date':None});plt.close(fig)
    fig,ax=plt.subplots(2,2,figsize=(13,8),layout='constrained');parcels=rows(a.audio/'parcels.csv')
    for weak,label in [('0','nominal12mA'),('1','weak6mA')]:
        r=[r for r in parcels if r['group']=='medium' and r['phase']=='0' and r['weak']==weak];x=[v*1e6 for v in values(r,'relative_s')];ax[0,0].plot(x,[v/1000 for v in values(r,'H_A_per_m')],'.-',label=label);ax[0,1].plot(x,values(r,'M'),'.-',label=label)
    ax[0,0].set(xlabel='parcel time before exit (μs)',ylabel='H (kA/m)',title='Actual reduced-model parcel events, same guitar frame');ax[0,0].legend();ax[0,1].set(xlabel='parcel time before exit (μs)',ylabel='m = M/Ms',title='Material history then exterior H=0 / remanence');ax[0,1].legend()
    spectra=rows(a.comparison/'carrier-spectra.csv')
    for weak,label in [('0','nominal'),('1','weak')]:
        r=[r for r in spectra if r['kind']=='hf' and r['weak']==weak and r['base_rate_hz']=='48000' and float(r['hz'])>0];ax[1,0].plot(values(r,'hz'),[20*math.log10(max(1e-9,float(v['reference_amplitude']))) for v in r],label=label+' resolved');ax[1,0].plot(values(r,'hz'),[20*math.log10(max(1e-9,float(v['native_amplitude']))) for v in r],ls='--',label=label+' native')
    ax[1,0].set(xlabel='Hz',ylabel='dB relative unit M',title='Hard10kHz/.8V: baseband + spurs (DC excluded)');ax[1,0].legend()
    levels=rows(a.audio/'renders.csv');groups=['low','medium','hard'];x=range(3)
    for signal,label in [('healthy','nominal'),('weakbias','weak')]:
        r=[next(r for r in levels if r['file']==f'{g}_{signal}_raw.wav') for g in groups];ax[1,1].plot(list(x),[20*math.log10(float(v['raw_rms'])) for v in r],'o-',label=label)
    ax[1,1].set(xticks=list(x),xticklabels=['.05V','.2V','.8V'],xlabel='same take03 input peak',ylabel='raw software RMS (dBFS)',title='Actual native musical levels; no matching gain here');ax[1,1].legend()
    fig.suptitle('SF012 — physical assumptions and model error remain explicit; listening approval pending');fig.savefig(a.output/'parcel-and-listening.svg',metadata={'Date':None});plt.close(fig)
    for path in a.output.glob('*.svg'):path.write_text('\n'.join(line.rstrip() for line in path.read_text().splitlines())+'\n')
if __name__=='__main__':main()
