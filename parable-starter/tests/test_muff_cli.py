"""Actual native audio, uniform recovery receipts, matching and source protection."""
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave


def run(*args,ok=True):
    r=subprocess.run([sys.argv[1],*map(str,args)],capture_output=True,text=True)
    assert (r.returncode==0)==ok,r.stdout+r.stderr


with tempfile.TemporaryDirectory(prefix='muff-cli-') as directory:
    root=Path(directory);source=root/'source.wav';rate=44100
    values=[]
    for i in range(rate):
        t=i/rate;age=t%.25
        values.append(round(7000*(1-math.exp(-age*800))*math.exp(-age*8)*(math.sin(2*math.pi*110*t)+.3*math.sin(2*math.pi*329*t))))
    with wave.open(str(source),'wb') as w:
        w.setparams((1,2,rate,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+str(len(values))+'h',*values))
    digest=hashlib.sha256(source.read_bytes()).hexdigest();one,two=root/'one',root/'two'
    for out in (one,two):run('--input',source,'--output',out)
    for p in one.iterdir():
        if p.name!='renders.csv':assert p.read_bytes()==(two/p.name).read_bytes(),p.name
    rows=list(csv.DictReader((one/'renders.csv').open()));assert len(rows)==21
    assert all(r['solver_failures']==r['output_containment']==r['clipped_samples']=='0' and float(r['final_peak'])<.999 for r in rows)
    assert max(float(r['final_rms']) for r in rows[3:6])-min(float(r['final_rms']) for r in rows[3:6])<1e-8
    assert all(float(r['offline_gain'])<=16.00001 for r in rows)
    assert all(float(r['offline_gain'])==1 for r in rows if r['file'].endswith('_raw.wav'))
    for k in range(3):assert rows[9+k*5]['offline_gain']==rows[10+k*5]['offline_gain']
    for p in one.glob('*.wav'):
        with wave.open(str(p)) as w:
            assert w.getnchannels()==1 and w.getsampwidth()==2
            expected=(rate,3*rate) if p.name[0].isdigit() else (48000,96000 if p.name.startswith('long_note') else 57600)
            assert (w.getframerate(),w.getnframes())==expected
    settings=json.loads((one/'settings.json').read_text());assert settings['fault_R15_ohms']==2200000 and settings['second_clip_reference']=='Q2'
    assert not settings['post_overload_suppression_proven']
    recovery={(r['fault'],r['prior_attacks'],r['start_s']):r for r in csv.DictReader((one/'recovery.csv').open())}
    ratio=float(recovery['1','1','0.09']['normalized_rms'])/float(recovery['1','0','0.09']['normalized_rms'])
    assert 1.05<ratio<1.5 # Real enhancement/priming, not falsely asserted suppression.
    state=list(csv.DictReader((one/'state.csv').open()))
    assert state and all(r['solver_failures']=='0' and abs(float(r['port_residual_V']))<=1.01e-9 for r in state)
    assert max(float(r['b2']) for r in state)>min(float(r['b2']) for r in state)+.1
    for name in ['00_dry_raw.wav','04_healthy_matched.wav','state.csv','settings.json','LISTEN.txt']:
        p=one/name;p.unlink();os.link(source,p);run('--input',source,'--output',one,ok=False)
        assert hashlib.sha256(source.read_bytes()).hexdigest()==digest;p.unlink()
    for args in [[],['--input'],['--unknown'],['--output',''],['--output',one,'--output',two]]:run(*args,ok=False)
    bad=root/'bad.wav';bad.write_bytes(b'not PCM');run('--input',bad,'--output',root/'bad-out',ok=False);assert not (root/'bad-out').exists()
print('Muff CLI:21 deterministic WAVs, disclosed gains/raw levels, true priming history, source safety; choke hypothesis remains unproven')
