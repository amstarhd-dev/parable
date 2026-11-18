import hashlib,json,math,struct,subprocess,sys,tempfile,wave
from pathlib import Path
root=Path(__file__).resolve().parents[1];lab=Path(sys.argv[1]);p=root/'third_party/tiled_room';pins=json.loads((p/'provenance.json').read_text())
for n,sha in pins['candidates'][0]['files'].items():assert hashlib.sha256((p/'upstream'/n).read_bytes()).hexdigest()==sha,n
for n,sha in pins['comparison']['sha256'].items():assert hashlib.sha256((p/'parablereference'/n).read_bytes()).hexdigest()==sha,n
with tempfile.TemporaryDirectory() as t:
 d=Path(t)
 def source(n,rate):
  with wave.open(str(n),'wb') as w:
   w.setparams((1,2,rate,rate,'NONE','not compressed'));w.writeframes(b''.join(struct.pack('<h',round(3000*math.sin(2*math.pi*131*i/rate))) for i in range(rate)))
 source(d/'good.wav',48000);source(d/'bad.wav',44100)
 for args in [[],['--input',str(d/'bad.wav'),'--output',str(d/'bad-out')],['--wrong','x','--output',str(d/'bad-out')]]:
  r=subprocess.run([str(lab),*args],capture_output=True);assert r.returncode!=0 and not (d/'bad-out').exists(),r.stderr
 # Actual final renderer, healthy and fault. Repeat the same meaningful input.
 for suffix in ['one','two']:
  subprocess.run([str(lab),'--input',str(d/'good.wav'),'--output',str(d/suffix)],check=True,capture_output=True)
 one,two=d/'one',d/'two';names=sorted(one.glob('*.wav'));assert len(names)==13,len(names)
 for f in names:assert f.read_bytes()==(two/f.name).read_bytes(),f.name
 assert (one/'healthy_raw.wav').read_bytes()!=(one/'fault_raw.wav').read_bytes()
 cfg=json.loads((one/'settings.json').read_text());assert cfg['dry']==.65 and cfg['wet']==.35 and cfg['fault_implemented']
print('Pinned imports, CLI boundaries,13 actual deterministic WAVs and nonzero principal dry passed')
