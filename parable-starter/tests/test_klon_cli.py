"""Actual native audio, controls, contact receipts and source protection."""
import csv,hashlib,json,math,os,struct,subprocess,sys,tempfile,wave
from pathlib import Path
def run(*args,ok=True):
    r=subprocess.run([sys.argv[1],*map(str,args)],capture_output=True,text=True)
    assert (r.returncode==0)==ok,r.stdout+r.stderr
with tempfile.TemporaryDirectory(prefix='klon-cli-') as temporary:
    root=Path(temporary);source=root/'source.wav';rate=44100
    values=[]
    for n in range(rate):
        time=n/rate;age=time%.25
        values.append(round(6000*(1-math.exp(-age*1200))*math.exp(-age*8)*(math.sin(2*math.pi*196*time)+.3*math.sin(2*math.pi*392*time))))
    with wave.open(str(source),'wb') as w:w.setparams((1,2,rate,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+str(len(values))+'h',*values))
    digest=hashlib.sha256(source.read_bytes()).hexdigest();out=root/'one';run('--input',source,'--output',out)
    # Every native render has its own exact reset/timed-repeat assertion. CI
    # separately repeats the entire real-DI runner and compares non-timing files.
    rows=list(csv.DictReader((out/'renders.csv').open()));assert len(rows)==48;by={r['file']:r for r in rows};assert len(by)==48
    assert all(r['solver_failures']==r['numerical_containment']==r['clipped_samples']=='0' for r in rows)
    assert all(float(r['final_peak'])<.999 and 0<float(r['offline_gain'])<=16.00001 for r in rows)
    assert all(float(r['offline_gain'])==1 for r in rows if r['file'].endswith('_raw.wav'))
    for group,label in enumerate(['low','medium','high']):
        matched=[by[f'{group}{k}_{label}_{signal}_matched.wav'] for k,signal in [(3,'dry'),(4,'healthy'),(5,'contact100k')]]
        assert max(float(r['final_rms']) for r in matched)-min(float(r['final_rms']) for r in matched)<1e-8
        h=by[f'{group}1_{label}_healthy_raw.wav'];f=by[f'{group}2_{label}_contact100k_raw.wav']
        assert h['input_gain_V_per_source_unit']==f['input_gain_V_per_source_unit'] and h['gain']==f['gain'] and h['treble']==f['treble'] and h['level']==f['level']
        assert float(h['raw_rms'])>1e-5 and float(f['raw_rms'])>1e-5
        assert (out/h['file']).read_bytes()!=(out/f['file']).read_bytes()
    for kind in ['attacks','long_note','impulse_release','chord']:assert by[kind+'_healthy_common.wav']['offline_gain']==by[kind+'_contact100k_common.wav']['offline_gain']
    assert len(set(r['offline_gain'] for r in rows if r['file'].startswith('analysis/')))==1
    settings=json.loads((out/'settings.json').read_text());assert settings['fault_contact_ohms']==100000 and settings['healthy_contact_ohms']==0 and settings['unchanged_drive_gang']=='P1B'
    assert settings['oversampling']==12 and settings['FIR_taps']==577 and settings['state_bytes']<=12288
    for p in out.rglob('*.wav'):
        with wave.open(str(p)) as w:
            assert (w.getnchannels(),w.getsampwidth())==(1,2)
            expected=(44100,3*44100) if p.name[0].isdigit() or p.parent.name=='analysis' or p.name.startswith(('low_guitar','high_guitar')) else (48000,96000 if p.name.startswith('long_note') else 144000 if p.name.startswith('chord') else 28800 if p.name.startswith('impulse_release') else 48000)
            assert (w.getframerate(),w.getnframes())==expected,p.name
    stages=list(csv.DictReader((out/'stage_stats.csv').open()));assert len(stages)==20
    assert all(float(r['KCL_peak_A'])<1e-10 and float(r['path_sum_peak_V'])<1e-5 for r in stages)
    assert all(3<float(r['Vref_min'])<=float(r['Vref_max'])<6 for r in stages)
    states=list(csv.DictReader((out/'state.csv').open()));assert states and all(r['solver_failures']=='0' and abs(float(r['KCL_A']))<1e-10 for r in states)
    assert all(float(r['last_phase11_s'])>(int(r['frame'])/int(r['rate_hz'])) for r in states)
    polarity=list(csv.DictReader((out/'polarity.csv').open()));assert len(polarity)>700
    hist=list(csv.DictReader((out/'histograms.csv').open()));assert len(hist)==20*64
    assert 'not the final Tone/Level WAV' in (out/'LISTEN.txt').read_text()
    for name in ['00_low_dry_raw.wav','15_medium_contact100k_matched.wav','analysis/medium_healthy_drive.wav','state.csv','settings.json','LISTEN.txt']:
        p=out/name;p.unlink();os.link(source,p);run('--input',source,'--output',out,ok=False);assert hashlib.sha256(source.read_bytes()).hexdigest()==digest;p.unlink()
    for args in [[],['--input'],['--unknown'],['--output',''],['--output',out,'--output',root/'two']]:run(*args,ok=False)
    bad=root/'bad.wav';bad.write_bytes(b'invalid');run('--input',bad,'--output',root/'bad-out',ok=False);assert not (root/'bad-out').exists()
print('Klon48 native WAVs, three gain conditions, actual single-contact loading, matched/raw levels, loaded path KCL and source safety')
