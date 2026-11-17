#!/usr/bin/env python3
"""Wallace host measurements and LINEAR loudness copies. No loudnorm/limiter."""
import argparse,csv,hashlib,json,math,re,subprocess,wave
from pathlib import Path
import numpy as np
p=argparse.ArgumentParser();p.add_argument('--directory',type=Path,required=True);p.add_argument('--lab',type=Path,required=True);a=p.parse_args();root=a.directory.resolve();lab=a.lab.resolve();meter={};settings=json.loads((root/'settings.json').read_text());assert settings['source_frames']==960000,'named study windows require supplied20s excerpt'
def samples(path):
 with wave.open(str(path),'rb') as w:
  assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(1,2,48000)
  return np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').astype(float)/32768
version=subprocess.check_output(['ffmpeg','-version'],text=True).splitlines()[0]
(root/'FFMPEG_VERSION.txt').write_text(version+'\n')
def measure(path):
 r=subprocess.run(['ffmpeg','-hide_banner','-nostats','-i',str(path),'-af','ebur128=peak=true:framelog=verbose','-f','null','-'],capture_output=True,text=True,check=True)
 (root/(path.stem+'.loudness.txt')).write_text(r.stderr)
 s=r.stderr.rsplit('Summary:',1)[1];I=float(re.search(r'\bI:\s*(-?[0-9.]+) LUFS',s)[1]);tp=float(re.search(r'Peak:\s*(-?[0-9.]+) dBFS',s)[1]);x=samples(path)
 return dict(integrated_lufs=I,true_peak_dbtp=tp,rms=float(np.sqrt(np.mean(x*x))),peak=float(max(abs(x))),mean=float(np.mean(x)))
for path in sorted(root.glob('*.wav')):meter[path.name]=measure(path)
principal=['03_A_serial_raw.wav','04_B_parallel_raw.wav','05_C_dynamic_raw.wav']
target=min([-23.]+[meter[n]['integrated_lufs']-1-meter[n]['true_peak_dbtp'] for n in principal]);matching={}
for name in principal+['08_C_no_carving.wav']:
 m=meter[name];gain=10**((target-m['integrated_lufs'])/20);dest=root/(name.removesuffix('_raw.wav').removesuffix('.wav')+'_lufs.wav')
 subprocess.run([lab,'--scale',root/name,dest,repr(gain)],check=True);result=measure(dest);assert abs(result['integrated_lufs']-target)<=.2;assert result['true_peak_dbtp']<=-.8
 matching[dest.name]=dict(source=name,linear_gain=gain,result=result);meter[dest.name]=result
# These two are NOT individually normalized: one identical listening gain retains
# the static/dynamic return attenuation. Raw06/07 files remain unchanged.
wetGain=matching['03_A_serial_lufs.wav']['linear_gain']
for name in ['06_B_wet.wav','07_C_wet.wav']:
 dest=root/(Path(name).stem+'_common_listen.wav');subprocess.run([lab,'--scale',root/name,dest,repr(wetGain)],check=True);meter[dest.name]=measure(dest)
# Active20s windowed spectra; energy fractions, not a perceived-quality score.
bands={'bass_20_120':[20,120],'low_mid_120_500':[120,500],'presence_500_2500':[500,2500],'upper_2500_12000':[2500,12000]}
spectra={};bandPowers={};window=np.hanning(4096);freq=np.fft.rfftfreq(4096,1/48000)
for name in principal+['08_C_no_carving.wav']:
 x=samples(root/name)[:960000];frames=np.lib.stride_tricks.sliding_window_view(x,4096)[::2048];power=np.mean(abs(np.fft.rfft(frames*window,axis=1))**2,axis=0);total=float(power[(freq>=20)&(freq<12000)].sum());spectra[name]={key:float(power[(freq>=lo)&(freq<hi)].sum()/total) for key,(lo,hi) in bands.items()};bandPowers[name]={key:float(power[(freq>=lo)&(freq<hi)].sum()) for key,(lo,hi) in bands.items()}
trace=list(csv.DictReader((root/'ducking.csv').open()));g=np.array([float(x['duck_gain']) for x in trace]);envelope=np.array([float(x['envelope']) for x in trace]);time=np.array([float(x['time_s']) for x in trace]);assert min(g)>=10**(-settings['ducking']['max_db']/20)-1e-6 and max(g)<=1+1e-6
# Windows refer to actual gaps in the selected unedited performance (source9.28s
# origin), plus final source/tail. No artificial gap or ducking trajectory.
windows=[('rhythmic_active',2,3),('gap_13s_source',3.88,4.26),('gap_23s_source',14.22,14.90),('sustained_decay',15,20),('tail_20_23',20,23)]
regions=[]
for label,start,end in windows:
 mask=(time>=start)&(time<end);entry=dict(name=label,start_s=start,end_s=end,min_gain=float(g[mask].min()),max_gain=float(g[mask].max()),mean_envelope=float(envelope[mask].mean()))
 for name in principal+['06_B_wet.wav','07_C_wet.wav']:
  x=samples(root/name)[int(start*48000):int(end*48000)];entry[name]=float(np.sqrt(np.mean(x*x)))
 regions.append(entry)
core=settings['parallel']['core']*samples(root/'02_muff.wav')
if settings['parallel']['clean']:
 dry=samples(root/'01_dry.wav');core+=settings['parallel']['clean']*np.r_[np.zeros(48),dry[:-48]]
wet=samples(root/'06_B_wet.wav');mix=samples(root/'04_B_parallel_raw.wav');powerCore=float(np.mean(core*core));powerWet=float(np.mean(wet*wet));mixPower=float(np.mean(mix*mix));coherence=dict(mix_over_incoherent_power_db=10*math.log10(mixPower/(powerCore+powerWet)),core_wet_cosine=float(np.mean(core*wet)/math.sqrt(powerCore*powerWet)),note='Actual aligned mono streams; intentional echo/rotor/room phase retained. PCM16 quantization means reconstruction is approximate.')
assert (root/'09_C_no_ducking.wav').read_bytes()==(root/'04_B_parallel_raw.wav').read_bytes()
report=dict(ffmpeg=version,numpy=np.__version__,meter='FFmpeg ebur128 BS1770 integrated gating and true-peak measurement; mono, not dual-mono compensation',loudness_resolution_lu=.1,target_lufs=target,matching=matching,wet_pair_common_gain=wetGain,levels=meter,active_spectral_energy_fractions=spectra,carving_absolute_band_power_delta_db={key:10*math.log10(bandPowers['05_C_dynamic_raw.wav'][key]/bandPowers['08_C_no_carving.wav'][key]) for key in bands},ducking=dict(min_gain=float(min(g)),max_gain=float(max(g)),max_reduction_db=float(-20*np.log10(min(g))),windows=regions),phase=coherence,limitations=['Integrated LUFS matching is a controlled perceptual proxy, not subjective equivalence.','Spectral/phase changes and envelope recovery do not establish musical superiority.','Full graph CPU is host-only; no Seed3 deadline/boot/audio claim.','No change to any of the four effect algorithms.'])
(root/'analysis.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k in ['target_lufs','ducking','phase']},indent=2))
with (root/'SHA256SUMS').open('w') as f:
 for path in sorted(root.iterdir()):
  if path.is_file() and path.name!='SHA256SUMS':f.write(hashlib.sha256(path.read_bytes()).hexdigest()+'  '+path.name+'\n')
