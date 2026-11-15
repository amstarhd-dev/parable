"""Actual delivered WAV/receipt/source preservation, not a mock renderer."""
import csv,hashlib,json,math,os,struct,subprocess,sys,tempfile,wave
from pathlib import Path
project=Path(__file__).resolve().parents[1];fixtures=project/'tests/fixtures/fz1'
for name,key,source in [('spice-manifest.json','fixtures','fz1_spice_reference.py'),('equation-manifest.json','fixtures','fz1_equation_reference.py')]:
    manifest=json.loads((fixtures/name).read_text());assert manifest['source_sha256']==hashlib.sha256((project/'tools'/source).read_bytes()).hexdigest()
    for p,h in manifest[key].items():assert hashlib.sha256((fixtures/p).read_bytes()).hexdigest()==h,p

m=json.loads((fixtures/'alias-manifest.json').read_text());assert m['reference_sha256']==hashlib.sha256((fixtures/'alias-reference.csv').read_bytes()).hexdigest()

def run(*args,ok=True):
    p=subprocess.run([sys.argv[1],*map(str,args)],capture_output=True,text=True)
    assert (p.returncode==0)==ok,p.stdout+p.stderr
with tempfile.TemporaryDirectory(prefix='fz1-cli-') as t:
    root=Path(t);source=root/'source.wav';rate=44100
    data=[round(6000*math.sin(2*math.pi*196*n/rate)*math.exp(-8*((n/rate)%.25))) for n in range(rate)]
    with wave.open(str(source),'wb') as w:w.setparams((1,2,rate,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+str(rate)+'h',*data))
    digest=hashlib.sha256(source.read_bytes()).hexdigest();out=root/'out';run('--input',source,'--output',out,'--compare-existing')
    rows=list(csv.DictReader((out/'renders.csv').open()));by={r['file']:r for r in rows};assert len(rows)==len(by)==102
    assert all(float(r['final_peak'])<.999 and 0<float(r['offline_gain'])<=16.000001 for r in rows)
    assert all(float(r['offline_gain'])==1 for r in rows if r['file'].endswith('_raw.wav'))
    for group in ['attack_low','attack_mid','attack_high','input_low','input_mid','input_high']:
        h=by[f'{group}_healthy_raw.wav'];f=by[f'{group}_feed_open_raw.wav'];assert h['input_gain_V_per_source_unit']==f['input_gain_V_per_source_unit'] and h['attack_fraction']==f['attack_fraction'];assert (out/h['file']).read_bytes()!=(out/f['file']).read_bytes()
        m=[float(by[f'{group}_{k}_rms_matched.wav']['final_rms']) for k in ['dry','healthy','feed_open']];assert max(m)-min(m)<1e-8
    for group in ['attack_low','attack_mid','attack_high','input_low','input_mid','input_high','pluck_decay','held_note','volume_rollback','hard_attack','chord','two_tone']:assert len({by[f'{group}_{k}_common.wav']['offline_gain'] for k in ['dry','healthy','feed_open']})==1
    for p in out.glob('*.wav'):
        with wave.open(str(p)) as w:
            assert w.getnchannels()==1 and w.getsampwidth()==2 and w.getnframes()>w.getframerate()
            samples=[v[0] for v in struct.iter_unpack('<h',w.readframes(w.getnframes()))]
        row=by[p.name];rms=math.sqrt(sum(v*v for v in samples)/len(samples))/32768;peak=max(abs(v) for v in samples)/32768
        assert abs(rms-float(row['final_rms']))<=1/32768 and abs(peak-float(row['final_peak']))<=1/32768
    cost=list(csv.DictReader((out/'performance.csv').open()));assert len(cost)==25 and all(r[k]=='0' for r in cost for k in ['failures','input_clamps','output_clamps']);assert all(int(r['max_evaluations'])<=193 for r in cost)
    state=list(csv.DictReader((out/'state.csv').open()));assert state and all(abs(float(r['residual_V']))<=1e-9 for r in state)
    for n in ['attack_low_healthy_raw.wav','input_high_feed_open_rms_matched.wav','compare_RAT_raw.wav','state.csv','settings.json','LISTEN.txt']:
        p=out/n;p.unlink();os.link(source,p);run('--input',source,'--output',out,ok=False);assert hashlib.sha256(source.read_bytes()).hexdigest()==digest;p.unlink()
    for args in [[],['--input'],['--unknown'],['--output',''],['--output',out,'--output',root/'two'],['--output',out,'--compare-existing','--compare-existing']]:run(*args,ok=False)
    bad=root/'bad.wav';bad.write_bytes(b'bad');run('--input',bad,'--output',root/'bad-out',ok=False);assert not(root/'bad-out').exists()
print('102 native WAVs, physical-state/level/reset receipts, matched pairs, source aliases, invalid CLI passed')
