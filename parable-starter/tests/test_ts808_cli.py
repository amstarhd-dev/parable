"""Real native DSP WAVs, electrical fault, matching, repeatability and source safety."""
import csv,hashlib,json,math,os,struct,subprocess,sys,tempfile,wave
from pathlib import Path
def run(*args,ok=True):
    r=subprocess.run([sys.argv[1],*map(str,args)],capture_output=True,text=True)
    assert (r.returncode==0)==ok,r.stdout+r.stderr
with tempfile.TemporaryDirectory(prefix='ts808-cli-') as temporary:
    root=Path(temporary);source=root/'source.wav';rate=44100
    values=[]
    for n in range(rate):
        time=n/rate;age=time%.25
        values.append(round(6000*(1-math.exp(-age*1200))*math.exp(-age*8)*(math.sin(2*math.pi*196*time)+.3*math.sin(2*math.pi*392*time))))
    with wave.open(str(source),'wb') as w:w.setparams((1,2,rate,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+str(len(values))+'h',*values))
    digest=hashlib.sha256(source.read_bytes()).hexdigest();one,two=root/'one',root/'two'
    for out in [one,two]:run('--input',source,'--output',out)
    for p in one.iterdir():
        if p.name!='renders.csv':assert p.read_bytes()==(two/p.name).read_bytes(),p.name
    rows=list(csv.DictReader((one/'renders.csv').open()));assert len(rows)==26
    assert all(r['solver_failures']==r['numerical_output_containment']==r['clipped_samples']=='0' for r in rows)
    assert all(float(r['final_peak'])<.999 and 0<float(r['offline_gain'])<=16.00001 for r in rows)
    assert max(float(r['final_rms']) for r in rows[3:6])-min(float(r['final_rms']) for r in rows[3:6])<1e-8
    assert all(float(r['offline_gain'])==1 for r in rows if r['file'].endswith('_raw.wav'))
    for k in range(4):assert rows[9+k*5]['offline_gain']==rows[10+k*5]['offline_gain']
    settings=json.loads((one/'settings.json').read_text());assert settings['fault_D2']=='true_open_anode_clip_cathode_minus' and settings['oversampling']==16 and settings['FIR_taps']==769 and settings['state_bytes']<=8192
    for p in one.glob('*.wav'):
        with wave.open(str(p)) as w:
            assert (w.getnchannels(),w.getsampwidth())==(1,2)
            expected=(44100,3*44100) if p.name[0].isdigit() else (48000,96000 if p.name.startswith('long_note') else 48000 if p.name.startswith('attacks') else 24000)
            assert (w.getframerate(),w.getnframes())==expected,p.name
    states=list(csv.DictReader((one/'state.csv').open()));assert states
    assert all(r['solver_failures']=='0' and abs(float(r['port_residual_V']))<1.01e-9 for r in states)
    assert all(float(r['D2_A'])==0 for r in states if r['fault']=='1')
    assert max(float(r['D2_A']) for r in states if r['fault']=='0')>1e-6
    stages={(r['source'],r['fault']):r for r in csv.DictReader((one/'stage_stats.csv').open())}
    assert float(stages['sine','1']['clip_max_V'])>float(stages['sine','0']['clip_max_V'])+1
    assert int(stages['sine','1']['physical_rail_samples'])>0
    assert float(stages['sine','1']['D2_peak_A'])==0
    polarity=list(csv.DictReader((one/'polarity.csv').open()));assert polarity
    assert all(float(r['last_phase15_s'])>float(r['frame_s']) for r in polarity)
    for name in ['00_dry_raw.wav','05_D2_open_matched.wav','polarity.svg','state.csv','settings.json','LISTEN.txt']:
        p=one/name;p.unlink();os.link(source,p);run('--input',source,'--output',one,ok=False)
        assert hashlib.sha256(source.read_bytes()).hexdigest()==digest;p.unlink()
    for args in [[],['--input'],['--unknown'],['--output',''],['--output',one,'--output',two]]:run(*args,ok=False)
    bad=root/'bad.wav';bad.write_bytes(b'invalid');run('--input',bad,'--output',root/'bad-out',ok=False);assert not (root/'bad-out').exists()
print('TS808:26 native WAVs, same source/settings, exact open D2, physical rails, disclosed matching, deterministic reset and source protection')
