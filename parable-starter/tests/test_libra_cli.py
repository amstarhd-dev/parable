"""Actual nine-file native A/B, level metadata, reference integrity and source protection."""
import csv,hashlib,json,math,os,struct,subprocess,sys,tempfile,wave
from pathlib import Path
root=Path(__file__).resolve().parents[1];fixtures=root/'tests/fixtures/libra';manifest=json.loads((fixtures/'reference.json').read_text())
for name,digest in manifest['fixtures'].items():assert hashlib.sha256((fixtures/name).read_bytes()).hexdigest()==digest,name
assert hashlib.sha256((root/'tools/libra_reference.py').read_bytes()).hexdigest()==manifest['generator_sha256']
def run(*args,ok=True):
 p=subprocess.run([sys.argv[1],*map(str,args)],capture_output=True,text=True);assert (p.returncode==0)==ok,p.stdout+p.stderr
with tempfile.TemporaryDirectory(prefix='libra-cli-') as temporary:
 directory=Path(temporary);source=directory/'input.wav';rate=44100
 samples=[round(6000*math.sin(2*math.pi*196*n/rate)*math.exp(-8*((n/rate)%.25))) for n in range(rate)]
 with wave.open(str(source),'wb') as w:w.setparams((1,2,rate,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+str(rate)+'h',*samples))
 digest=hashlib.sha256(source.read_bytes()).hexdigest();out=directory/'audio';run('--input',source,'--output',out);rows=list(csv.DictReader((out/'renders.csv').open()));by={r['file']:r for r in rows};assert len(rows)==len(by)==len(list(out.glob('*.wav')))==9
 for r in rows:
  assert r['failures']==r['input_clamps']=='0' and 0<float(r['offline_gain'])<=16.00001
  with wave.open(str(out/r['file'])) as w:
   assert w.getnchannels()==1 and w.getsampwidth()==2;x=[v[0]/32768 for v in struct.iter_unpack('<h',w.readframes(w.getnframes()))]
  assert abs(math.sqrt(sum(v*v for v in x)/len(x))-float(r['final_rms']))<1/32768 and abs(max(map(abs,x))-float(r['final_peak']))<1/32768
 for group,drive in [('low',.1),('high',.65)]:
  for kind in ['raw','matched']:
   h=by[f'{group}_healthy_{kind}.wav'];f=by[f'{group}_RB1_270k_{kind}.wav'];assert h['level']==f['level'] and h['drive']==f['drive'] and abs(float(h['drive'])-drive)<1e-6
   assert (out/h['file']).read_bytes()!=(out/f['file']).read_bytes()
   if kind=='raw':assert h['offline_gain']==f['offline_gain']=='1'
   else:assert abs(float(h['final_rms'])-float(f['final_rms']))<1e-8
 assert int(by['low_healthy_raw.wav']['rail_samples'])==0 and int(by['high_healthy_raw.wav']['rail_samples'])>0
 protected=out/'high_healthy_raw.wav';protected.unlink();os.link(source,protected);run('--input',source,'--output',out,ok=False);assert hashlib.sha256(source.read_bytes()).hexdigest()==digest
 for args in [[],['--unknown'],['--input',source,'--output',source]]:run(*args,ok=False)
print('9 native WAVs: raw/matched healthy/RB1 pairs, clean/overdriven controls, PCM metadata, source and fixture integrity pass')
