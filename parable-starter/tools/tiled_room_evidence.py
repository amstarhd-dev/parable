#!/usr/bin/env python3
"""External FFmpeg BS1770 measurement and constant-gain listening copies only."""
import argparse,hashlib,json,math,re,subprocess,zipfile,wave
import numpy as np
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--directory',type=Path,required=True);a=p.parse_args();root=a.directory
def samples(path):
 with wave.open(str(path),'rb') as w:
  assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(1,2,48000)
  return np.frombuffer(w.readframes(w.getnframes()),'<i2').astype(float)/32768
comparison={}
if (root/'comparison_roomA_wet_raw.wav').exists():
 h=samples(root/'healthy_wet_raw.wav');old=samples(root/'comparison_roomA_wet_raw.wav');dry=samples(root/'dry_raw.wav')
 factor=float(np.linalg.norm(h)/np.linalg.norm(old));mix=.65*dry+.35*factor*old;assert max(abs(mix))<.98
 with wave.open(str(root/'comparison_roomA_raw.wav'),'wb') as w:
  w.setparams((1,2,48000,len(mix),'NONE','not compressed'));w.writeframes(np.rint(mix*32768).astype('<i2').tobytes())
 comparison=dict(source='exact existing SF020 Room A, aperture0, original mono readout',native_nominal_rt60_s=.65,offline_wet_rms_gain=factor,mix_law='.65 calibrated source + .35 scaled existing room return',note='Scalar equalizes wet RMS to healthy Tiled for this one comparison only; unscaled actual old wet return retained.')
names=['healthy_raw']+(['fault_raw'] if (root/'fault_raw.wav').exists() else [])+(['comparison_roomA_raw'] if (root/'comparison_roomA_raw.wav').exists() else [])
def measure(path):
 r=subprocess.run(['ffmpeg','-hide_banner','-nostats','-i',str(path),'-af','ebur128=peak=true:framelog=verbose','-f','null','-'],capture_output=True,text=True,check=True)
 (root/(path.stem+'.loudness.txt')).write_text(r.stderr);s=r.stderr.rsplit('Summary:',1)[1]
 return dict(lufs=float(re.search(r'\bI:\s*(-?[0-9.]+) LUFS',s)[1]),dbtp=float(re.search(r'Peak:\s*(-?[0-9.]+) dBFS',s)[1]))
def scale(src,dest,gain):
 subprocess.run(['ffmpeg','-v','error','-y','-i',str(src),'-af',f'volume={gain:.17g}:precision=double','-c:a','pcm_s16le','-fflags','+bitexact','-flags:a','+bitexact','-map_metadata','-1',str(dest)],check=True)
levels={n:measure(root/(n+'.wav')) for n in names};target=min([-23.]+[m['lufs']-1-m['dbtp'] for m in levels.values()]);matching={}
for name in names:
 m=levels[name];gain=10**((target-m['lufs'])/20);prefix=name.removesuffix('_raw');dst=root/(prefix+'_matched.wav');scale(root/(name+'.wav'),dst,gain);matched=measure(dst);assert abs(matched['lufs']-target)<=.2 and matched['dbtp']<=-.8
 matching[prefix]=dict(gain=gain,source=m,matched=matched)
 scale(root/(prefix+'_wet_raw.wav'),root/(prefix+'_wet_listen.wav'),gain*(comparison.get('offline_wet_rms_gain',1) if prefix=='comparison_roomA' else 1))
 if (root/(prefix+'_stereo_raw.wav')).exists():
  m=measure(root/(prefix+'_stereo_raw.wav'));g=10**((target-m['lufs'])/20);scale(root/(prefix+'_stereo_raw.wav'),root/(prefix+'_stereo_matched.wav'),g);matching[prefix]['stereo_gain']=g;matching[prefix]['stereo_matched']=measure(root/(prefix+'_stereo_matched.wav'));assert abs(matching[prefix]['stereo_matched']['lufs']-target)<=.2 and matching[prefix]['stereo_matched']['dbtp']<=-.8
if (root/'transient_fault.wav').exists():
 for prefix in ['healthy','fault']:scale(root/('transient_'+prefix+'.wav'),root/('transient_'+prefix+'_common_listen.wav'),matching['healthy']['gain'])
mono_checks={}
for prefix in ['healthy','fault']:
 path=root/(prefix+'_stereo_raw.wav')
 if not path.exists():continue
 with wave.open(str(path),'rb') as w:
  pair=np.frombuffer(w.readframes(w.getnframes()),'<i2').astype(float).reshape(-1,2)/32768
 mono=samples(root/(prefix+'_raw.wav'));dry=samples(root/'dry_raw.wav');error=float(max(abs(pair.mean(axis=1)-mono)));assert error<=1/32768
 wet=(pair-.65*dry[:,None])/.35
 cosine=float(np.dot(wet[:,0],wet[:,1])/np.linalg.norm(wet[:,0])/np.linalg.norm(wet[:,1]))
 power=float(np.mean(wet.mean(axis=1)**2)/np.mean(wet**2))
 mono_checks[prefix]=dict(max_pcm_mono_average_error=error,wet_left_right_cosine=cosine,mono_wet_power_vs_stereo_mean_db=10*math.log10(power),note='Wet inferred from quantized full stereo and common dry path; no subjective width/quality claim.')
report=dict(mono_checks=mono_checks,comparison= comparison,matching=matching,target_lufs=target,meter='external FFmpeg ebur128; measured0.1LU resolution; constant gains only; scaled rawPCM16 exports',ffmpeg=subprocess.check_output(['ffmpeg','-version'],text=True).splitlines()[0],fault_present='fault_raw' in names)
(root/'matching.json').write_text(json.dumps(report,indent=2)+'\n')
archive=root/'SF025-listening.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for path in sorted(root.iterdir()):
  if path.is_file() and path.suffix in ['.wav','.md','.json','.txt','.csv'] and not path.name.startswith('source-original'):z.write(path,'SF025-listening/'+path.name)
with zipfile.ZipFile(archive) as z:
 assert z.testzip() is None
 for name in z.namelist():assert z.read(name)==(root/Path(name).name).read_bytes(),name
with (root/'SHA256SUMS').open('w') as f:
 for path in sorted(root.iterdir()):
  if path.is_file() and path.name!='SHA256SUMS':f.write(hashlib.sha256(path.read_bytes()).hexdigest()+'  '+path.name+'\n')
print(json.dumps(report,indent=2));print('ZIP',archive.stat().st_size)
