"""Nine native WAVs, meaningful pairs, explicit gains, authentic fixtures and input safety."""
import csv,hashlib,json,math,os,struct,subprocess,sys,tempfile,wave
from pathlib import Path
root=Path(__file__).resolve().parents[1];fixtures=root/'tests/fixtures/capricorn';manifest=json.loads((fixtures/'reference.json').read_text())
for name,digest in manifest['fixtures'].items():assert hashlib.sha256((fixtures/name).read_bytes()).hexdigest()==digest,name
assert hashlib.sha256((root/'tools/capricorn_reference.py').read_bytes()).hexdigest()==manifest['generator_sha256']
def run(*args,ok=True):
    r=subprocess.run([sys.argv[1],*map(str,args)],capture_output=True,text=True);assert (r.returncode==0)==ok,r.stdout+r.stderr
with tempfile.TemporaryDirectory(prefix='capricorn-cli-') as temporary:
    directory=Path(temporary);source=directory/'input.wav';rate=44100
    values=[round(6000*(math.sin(2*math.pi*196*n/rate)+.2*math.sin(2*math.pi*3200*n/rate))*math.exp(-8*((n/rate)%.25))) for n in range(rate)]
    with wave.open(str(source),'wb') as w:w.setparams((1,2,rate,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+str(rate)+'h',*values))
    digest=hashlib.sha256(source.read_bytes()).hexdigest();out=directory/'audio';run('--input',source,'--output',out)
    rows=list(csv.DictReader((out/'renders.csv').open()));by={r['file']:r for r in rows};assert len(rows)==len(by)==len(list(out.glob('*.wav')))==9
    for r in rows:
        assert all(r[k]=='0' for k in ['failures','input_clamps','table_clamps']);assert 0<float(r['offline_gain'])<=16.00001
        with wave.open(str(out/r['file'])) as w:
            assert w.getnchannels()==1 and w.getsampwidth()==2;samples=[x[0]/32768 for x in struct.iter_unpack('<h',w.readframes(w.getnframes()))]
        assert abs(math.sqrt(sum(x*x for x in samples)/len(samples))-float(r['final_rms']))<1/32768
        assert abs(max(map(abs,samples))-float(r['final_peak']))<1/32768
    for group,distortion in [('low',.15),('high',.9)]:
        for kind in ['raw','matched']:
            h=by[f'{group}_healthy_{kind}.wav'];f=by[f'{group}_C4_open_{kind}.wav'];assert h['tone']==f['tone'] and h['level']==f['level'] and abs(float(h['distortion'])-distortion)<1e-6 and h['distortion']==f['distortion']
            assert (out/h['file']).read_bytes()!=(out/f['file']).read_bytes()
            if kind=='raw':assert h['offline_gain']==f['offline_gain']=='1'
            else:assert abs(float(h['final_rms'])-float(f['final_rms']))<1e-8
    assert float(by['high_healthy_raw.wav']['raw_rms'])>float(by['low_healthy_raw.wav']['raw_rms'])
    protected=out/'high_healthy_raw.wav';protected.unlink();os.link(source,protected);run('--input',source,'--output',out,ok=False);assert hashlib.sha256(source.read_bytes()).hexdigest()==digest
    for args in [[],['--unknown'],['--input',source,'--output',source]]:run(*args,ok=False)
print('9 native WAVs: raw/matched healthy/C4 pairs, controls, PCM levels, fixture/generator integrity and source protection pass')
