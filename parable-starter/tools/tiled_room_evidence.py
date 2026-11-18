#!/usr/bin/env python3
"""External FFmpeg BS1770 measurement and constant-gain listening copies only."""
import argparse,hashlib,json,math,re,subprocess,zipfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--directory',type=Path,required=True);a=p.parse_args();root=a.directory
names=['healthy_raw']+(['fault_raw'] if (root/'fault_raw.wav').exists() else [])
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
 scale(root/(prefix+'_wet_raw.wav'),root/(prefix+'_wet_listen.wav'),gain)
 if (root/(prefix+'_stereo_raw.wav')).exists():
  m=measure(root/(prefix+'_stereo_raw.wav'));g=10**((target-m['lufs'])/20);scale(root/(prefix+'_stereo_raw.wav'),root/(prefix+'_stereo_matched.wav'),g);matching[prefix]['stereo_gain']=g
report=dict(matching=matching,target_lufs=target,meter='external FFmpeg ebur128; measured0.1LU resolution; constant gains only; scaled rawPCM16 exports',ffmpeg=subprocess.check_output(['ffmpeg','-version'],text=True).splitlines()[0],fault_present='fault_raw' in names)
(root/'matching.json').write_text(json.dumps(report,indent=2)+'\n')
archive=root/'SF025-listening.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for path in sorted(root.iterdir()):
  if path.is_file() and path.suffix in ['.wav','.md','.json','.txt'] and not path.name.startswith('source-original'):z.write(path,'SF025-listening/'+path.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
with (root/'SHA256SUMS').open('w') as f:
 for path in sorted(root.iterdir()):
  if path.is_file() and path.name!='SHA256SUMS':f.write(hashlib.sha256(path.read_bytes()).hexdigest()+'  '+path.name+'\n')
print(json.dumps(report,indent=2));print('ZIP',archive.stat().st_size)
