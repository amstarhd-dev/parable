"""Actual same-C++ WAVs: deterministic receipts, contact distinction, source safety."""
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
    r=subprocess.run([sys.argv[1],*map(str,args)],text=True,capture_output=True)
    assert (r.returncode==0)==ok,r.stdout+r.stderr


with tempfile.TemporaryDirectory(prefix='spring-cli-') as directory:
    root=Path(directory);source=root/'source.wav';rate=32000;frames=rate
    # Low-frequency, finite transients move the suspension; not a guitar fixture.
    values=[round(12000*math.sin(math.pi*(i%800)/160)) if i%800<160 else 0 for i in range(frames)]
    with wave.open(str(source),'wb') as w:
        w.setparams((1,2,rate,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+str(frames)+'h',*values))
    digest=hashlib.sha256(source.read_bytes()).hexdigest();one,two=root/'one',root/'two'
    for output in (one,two):run('--input',source,'--output',output)
    for p in one.iterdir():
        if p.name!='renders.csv':assert p.read_bytes()==(two/p.name).read_bytes(),p.name
    rows=list(csv.DictReader((one/'renders.csv').open()));assert len(rows)==21
    assert max(float(r['final_rms']) for r in rows[3:6])-min(float(r['final_rms']) for r in rows[3:6])<1e-8
    assert all(float(r['final_peak'])<.999 and r['clipped_samples']==r['port_limits']==r['numerical_faults']=='0' for r in rows)
    assert all(r['offline_gain']=='1' for r in rows if r['file'].endswith('_raw.wav'))
    assert all(r['offline_gain']=='64' for r in rows if 'common64' in r['file'])
    for p in one.glob('*.wav'):
        with wave.open(str(p)) as w:
            assert w.getnchannels()==1 and w.getsampwidth()==2
            assert (w.getframerate(),w.getnframes())==((rate,7*rate) if p.name[0].isdigit() else (48000,288240))
    comparison={r['probe']:r for r in csv.DictReader((one/'comparison.csv').open())}
    assert comparison['weak']['fault_impacts']=='0' and float(comparison['weak']['raw_difference_rms'])==0
    assert all(r['healthy_impacts']=='0' for r in comparison.values())
    assert int(comparison['strong']['fault_impacts'])>10 and float(comparison['strong']['max_impact_HF_gain_J'])>1e-12
    assert int(comparison['music']['fault_impacts'])>0
    bands={}
    for r in csv.DictReader((one/'spectrum.csv').open()):
        total,high=bands.get(r['signal'],(0,0));power=float(r['peak_amplitude'])**2
        bands[r['signal']]=(total+power,high+(power if float(r['frequency_hz'])>=2000 else 0))
    healthy=bands['strong_healthy'];fault=bands['strong_contact']
    assert fault[1]/fault[0]>.001 and fault[1]/fault[0]>100*healthy[1]/healthy[0]
    settings=json.loads((one/'settings.json').read_text());assert settings['fault_gap_m']==.000005 and settings['modes']==192
    trace=list(csv.DictReader((one/'energy.csv').open()))
    impacts=[r for r in trace if int(r['impacts'])]
    assert impacts and all(float(r['peak_pre_displacement_m'])>.000005 and float(r['last_contact_u_before_m_s'])>0 for r in impacts)
    assert all(abs(float(r['balance_error_J']))<1e-18 for r in trace)
    alias=list(csv.DictReader((one/'alias.csv').open()));assert len(alias)==6
    assert all(0<=float(r['HF_above_16k_power_fraction'])<=1 for r in alias)
    for name in ['00_dry_raw.wav','04_healthy_matched.wav','energy.csv','settings.json','LISTEN.txt']:
        p=one/name;p.unlink();os.link(source,p);run('--input',source,'--output',one,ok=False)
        assert hashlib.sha256(source.read_bytes()).hexdigest()==digest;p.unlink()
    for args in [[],['--input'],['--unknown'],['--output',''],['--output',one,'--output',two]]:run(*args,ok=False)
    bad=root/'bad.wav';bad.write_bytes(b'not PCM');run('--input',bad,'--output',root/'bad-out',ok=False)
    assert not (root/'bad-out').exists()
print('Spring CLI:21 deterministic WAVs; raw/matched levels, passive/contact traces, alias-risk receipts and input protection passed')
