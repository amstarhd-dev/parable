#!/usr/bin/env python3
"""New host boundary/source-lock checks, no repeated processor characterization."""
import hashlib,json,math,struct,subprocess,sys,tempfile,wave
from pathlib import Path
exe=Path(sys.argv[1]).resolve();root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'third_party/wallace/hall-import.json').read_text())
for name,digest in manifest['files_sha256'].items():assert hashlib.sha256((root.parent/name).read_bytes()).hexdigest()==digest,name
assert manifest['commit']=='21905d5ca171c1cb98d2e228dda33beac039ac4a'
with tempfile.TemporaryDirectory() as d:
 d=Path(d);source=d/'input.wav';out=d/'scaled.wav'
 with wave.open(str(source),'wb') as w:
  w.setparams((1,2,48000,0,'NONE','not compressed'));w.writeframes(b''.join(struct.pack('<h',int(3000*math.sin(2*math.pi*131*i/48000))) for i in range(48000)))
 before=source.read_bytes()
 for cmd in [[],['--input',d/'absent.wav','--output',d/'bad'],['--input',source,'--output',d/'bad','--duck-db','7'],['--input',source,'--output',d/'bad','--send-hall','nan'],['--scale',source,out,'nan']]:assert subprocess.run([exe,*map(str,cmd)],capture_output=True).returncode!=0,cmd
 subprocess.run([exe,'--scale',source,out,'1.5'],check=True)
 with wave.open(str(out),'rb') as w:
  assert w.getparams()[:3]==(1,2,48000);b=struct.unpack('<48000h',w.readframes(48000))
 with wave.open(str(source),'rb') as w:a=struct.unpack('<48000h',w.readframes(48000))
 assert max(abs(y-1.5*x) for x,y in zip(a,b))<=.5
 assert source.read_bytes()==before
print('Exact unmodified Hall import, invalid controls/input and constant-gain PCM boundary passed')
