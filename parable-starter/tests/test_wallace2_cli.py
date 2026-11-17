"""No NumPy/FFmpeg/network needed for these host safety/provenance tests."""
import hashlib,json,subprocess,sys,tempfile,wave,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1];lab=Path(sys.argv[1]);repo=root.parent
lock=json.loads((root/'docs/data/sf024/protected-source.json').read_text())
for n,sha in lock['sha256'].items():assert hashlib.sha256((repo/n).read_bytes()).hexdigest()==sha,n
lock=json.loads((root/'third_party/fftconvolver/UPSTREAM.json').read_text())
for n,sha in lock['local_sha256'].items():assert hashlib.sha256((root/'third_party/fftconvolver'/n).read_bytes()).hexdigest()==sha,n
prep=json.loads((root/'samples/wallace2/irs/preparation.json').read_text())
for n,m in prep['irs'].items():
 for suffix,key in [('-original.wav','original_sha256'),('-48k.f32','kernel_sha256')]:assert hashlib.sha256((root/'samples/wallace2/irs'/(n+suffix)).read_bytes()).hexdigest()==m[key]
with tempfile.TemporaryDirectory() as t:
 d=Path(t);src=d/'input.wav'
 with wave.open(str(src),'wb') as w:w.setparams((1,2,48000,48000,'NONE','not compressed'));w.writeframes(struct.pack('<h',1000)*48000)
 args=[str(lab),'--input',str(src),'--irs',str(root/'samples/wallace2/irs'),'--output',str(d/'out')]
 for flag,value in [('--clean','0'),('--clean','nan'),('--muff','inf'),('--tape','-1'),('--unknown','1')]:
  p=subprocess.run(args+[flag,value],capture_output=True);assert p.returncode!=0 and not (d/'out').exists(),(flag,p.stderr)
print('Protected native/firmware/fixture bytes, upstream/IR pins and CLI rejection passed')
