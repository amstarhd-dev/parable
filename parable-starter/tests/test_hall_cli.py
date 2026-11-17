#!/usr/bin/env python3
"""Audition container, invalid CLI/input, and deterministic recording checks."""
import hashlib,json,math,struct,subprocess,sys,tempfile,wave
from pathlib import Path
exe=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as d:
 root=Path(d);source=root/'input.wav'
 with wave.open(str(source),'wb') as w:
  w.setparams((1,2,48000,0,'NONE','not compressed'))
  w.writeframes(b''.join(struct.pack('<h',int(3000*math.sin(2*math.pi*131*i/48000))) for i in range(48000)))
 assert subprocess.run([exe],capture_output=True).returncode!=0
 assert subprocess.run([exe,'--input',root/'missing.wav','--output',root/'bad'],capture_output=True).returncode!=0
 for folder in ['one','two']:
  subprocess.run([exe,'--input',source,'--output',root/folder],check=True,capture_output=True)
 one=root/'one';two=root/'two';files=list(one.glob('*.wav'));assert len(files)==18,len(files)
 for p in files:
  assert p.read_bytes()==(two/p.name).read_bytes(),p.name
  with wave.open(str(p),'rb') as w:
   assert w.getframerate()==48000 and w.getnframes()==16*48000
   assert w.getsampwidth()==2 and w.getnchannels()==(2 if 'stereo' in p.name else 1)
 assert json.loads((one/'settings.json').read_text())['fixed_instance_bytes']<512*1024
 assert (one/'healthy_raw.wav').read_bytes()!=(one/'fault_raw.wav').read_bytes()
 print('18 WAVs deterministic, bounded PCM containers; CLI rejection passed')
