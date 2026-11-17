#!/usr/bin/env python3
"""Use existing external FFmpeg LUFS method; constant gains, no compressor/limiter."""
import argparse,hashlib,json,math,re,subprocess,wave
from pathlib import Path
import numpy as np
p=argparse.ArgumentParser();p.add_argument('--directory',type=Path,required=True);p.add_argument('--discard-floats',action='store_true');p.add_argument('--verify-pcm-only',action='store_true');a=p.parse_args();root=a.directory
names=['01_muff_direct','02_muff_jensen','03_muff_v30','04_original_serial','05_pre_cab_clean','06_CONTROL_clean_disabled','07_more_clean_less_muff','08_post_cab_studio','09_post_cab_duck','10_expansive']
def verify_pcm():
 def pcm(name):
  with wave.open(str(root/(name+'.wav')),'rb') as w:
   assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(1,2,48000)
   return np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').astype(float)/32768
 result={}
 for n in names[4:]:
  if 'CONTROL' in n:continue
  full=pcm(n+'_matched');total=sum(pcm(n+'_stem_'+key+'_listen') for key in ['clean_after_cab','fuzz_after_cab','ambience_final'])
  e=float(max(abs(full-total)));assert e<=3/32768
  result[n]=dict(max_pcm_stem_sum_error=e,lsb=e*32768)
 e=float(max(abs(pcm(names[4]+'_common_gain')-pcm(names[5]+'_common_gain')-pcm(names[4]+'_stem_clean_after_cab_listen'))));assert e<=2/32768
 result['clean_off_difference']=dict(max_pcm_error=e,lsb=e*32768)
 return result
if a.verify_pcm_only:
 report=json.loads((root/'analysis.json').read_text());report['published_pcm_verification']=verify_pcm();(root/'analysis.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report['published_pcm_verification'],indent=2));raise SystemExit(0)
assert json.loads((root/'settings.json').read_text())['active_frames']==960000,'this study uses the identical20s SF023 excerpt'
read=lambda n:np.fromfile(root/(n+'.f32'),dtype='<f4').astype(float)
def measure(path,raw=False):
 cmd=['ffmpeg','-hide_banner','-nostats']
 if raw:cmd+=['-f','f32le','-ar','48000','-ac','1']
 cmd+=['-i',str(path),'-af','ebur128=peak=true:framelog=verbose','-f','null','-']
 r=subprocess.run(cmd,capture_output=True,text=True,check=True);(root/(path.stem+('.float' if raw else '')+'.loudness.txt')).write_text(r.stderr)
 s=r.stderr.rsplit('Summary:',1)[1];return dict(lufs=float(re.search(r'\bI:\s*(-?[0-9.]+) LUFS',s)[1]),dbtp=float(re.search(r'Peak:\s*(-?[0-9.]+) dBFS',s)[1]))
def matched(src,dst,gain):
 subprocess.run(['ffmpeg','-v','error','-y','-f','f32le','-ar','48000','-ac','1','-i',str(src),'-af',f'volume={gain:.17g}:precision=double','-c:a','pcm_s16le','-fflags','+bitexact','-flags:a','+bitexact','-map_metadata','-1',str(dst)],check=True)
levels={n:measure(root/(n+'.f32'),True) for n in names}
target=min([-23.]+[m['lufs']-1-m['dbtp'] for m in levels.values()]);matches={}
for name in names:
 gain=10**((target-levels[name]['lufs'])/20);dest=root/(name+'_matched.wav');matched(root/(name+'.f32'),dest,gain);m=measure(dest)
 assert abs(m['lufs']-target)<=.2 and m['dbtp']<=-.8,(name,m)
 matches[name]=dict(gain=gain,**m)
# Shared fullmix listening gain for every stem of that recipe: never solo-normalize
# clean to exaggerate its contribution, and never normalize away control attenuation.
metrics={};rms=lambda x:float(np.sqrt(np.mean(x*x)))
for n in names[4:]:
 if 'CONTROL' in n:continue
 full=read(n);stems={key:read(n+'_stem_'+key) for key in ['clean_before_cab','clean_after_cab','fuzz_after_cab','ambience_final']}
 reconstructed=stems['clean_after_cab']+stems['fuzz_after_cab']+stems['ambience_final']
 error=float(max(abs(full-reconstructed)));assert error<2e-6
 clean=stems['clean_after_cab'];assert rms(clean[:960000])>.001
 fuzz=stems['fuzz_after_cab'];correlation=float(np.dot(clean,fuzz)/np.linalg.norm(clean)/np.linalg.norm(fuzz))
 metrics[n]=dict(stem_sum_max_error=error,active_clean_rms=rms(clean[:960000]),active_full_rms=rms(full[:960000]),clean_vs_full_rms_db=20*math.log10(rms(clean[:960000])/rms(full[:960000])),clean_fuzz_cosine=correlation,clean_fuzz_power_vs_incoherent_db=10*math.log10(rms(clean+fuzz)**2/(rms(clean)**2+rms(fuzz)**2)))
 for key in stems:matched(root/(n+'_stem_'+key+'.f32'),root/(n+'_stem_'+key+'_listen.wav'),matches[n]['gain'])
controlError=float(max(abs((read(names[4])-read(names[5]))-read(names[4]+'_stem_clean_after_cab'))));assert controlError<2e-6
# One shared gain for clean-on/control and original/rebalanced variants also exposes
# their raw gain differences; separate individually matched versions stay available.
g=matches[names[4]]['gain']
for n in names[4:7]:matched(root/(n+'.f32'),root/(n+'_common_gain.wav'),g)
# Independent NumPy whole-signal convolution reference for cabinet-only A/B and
# actual weighted clean stems. C++ tests separately use direct time-domain math.
references={}
for name,ir in [(names[1],'jensen'),(names[2],'v30')]:
 h=np.fromfile(root/'irs'/(ir+'-48k.f32'),dtype='<f4').astype(float)
 # Short physical probe separately covered in C++; long recording uses independent
 # NumPy FFT convolution, not a second call to the borrowed processor.
 x=read(names[0]);size=1<<(len(x)+len(h)-2).bit_length();expected=np.fft.irfft(np.fft.rfft(x,size)*np.fft.rfft(h,size),size)[:len(x)]
 err=read(name)-expected;references[name]=dict(max_error=float(max(abs(err))),relative_rms=float(np.linalg.norm(err)/np.linalg.norm(expected)));assert references[name]['max_error']<8e-6
# Measured cabinet magnitude response, not a musical score.
response={}
for name in ['jensen','v30']:
 h=np.fromfile(root/'irs'/(name+'-48k.f32'),dtype='<f4').astype(float);s=abs(np.fft.rfft(h,262144));f=np.fft.rfftfreq(262144,1/48000)
 response[name]={str(hz):20*math.log10(max(float(np.interp(hz,f,s)),1e-15)) for hz in [80,200,1000,3000,6000,10000,16000]}
report=dict(target_lufs=target,raw_float_levels=levels,matched=matches,clean_stems=metrics,clean_off_difference_max_error=controlError,independent_whole_recording_convolution=references,cab_response_db=response,ffmpeg=subprocess.check_output(['ffmpeg','-version'],text=True).splitlines()[0],numpy=np.__version__,limitations=['Perceived authenticity/articulation and pre/post preference remain listening judgments.','Clean solos retain the fullmix gain; clean is cabinet-colored but bypasses all effects.','Measured host cost is not Seed3 admission; no live hardware run.'])
report['published_pcm_verification']=verify_pcm();(root/'analysis.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(target_lufs=target,clean_stems=metrics,clean_off_error=controlError),indent=2))
if a.discard_floats:
 for path in root.glob('*.f32'):path.unlink() # reproducible intermediates, never original IRs
with (root/'SHA256SUMS').open('w') as f:
 for path in sorted(root.rglob('*')):
  if path.is_file() and path.name!='SHA256SUMS':f.write(hashlib.sha256(path.read_bytes()).hexdigest()+'  '+str(path.relative_to(root))+'\n')
