"""Actual tape runner: deterministic WAV/proof receipts and source protection."""
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


with tempfile.TemporaryDirectory(prefix='tape-cli-') as directory:
    root=Path(directory);source=root/'source.wav'
    with wave.open(str(source),'wb') as w:
        w.setparams((1,2,44100,0,'NONE','not compressed'))
        w.writeframes(struct.pack('<16384h',*[round(5000*math.sin(2*math.pi*(440 if i<8192 else 660)*i/44100)) for i in range(16384)]))
    original=hashlib.sha256(source.read_bytes()).hexdigest();first,second=root/'one',root/'two'
    for out in (first,second):run('--input',source,'--output',out)
    for p in first.iterdir():
        if p.name!='renders.csv':assert p.read_bytes()==(second/p.name).read_bytes(),p.name
    rows=list(csv.DictReader((first/'renders.csv').open()))
    assert len(rows)==6
    assert max(float(x['final_rms']) for x in rows[3:])-min(float(x['final_rms']) for x in rows[3:])<1e-8
    assert all(float(x['final_peak'])<=.890001 for x in rows)
    assert rows[1]['retention']=='0' and abs(float(rows[2]['retention'])-.85)<1e-7
    settings=json.loads((first/'settings.json').read_text())
    assert (settings['loop_cells'],settings['write_to_play_cells'],settings['erase_to_write_cells'])==(52920,13230,662)
    ghosts=list(csv.DictReader((first/'ghost.csv').open()))
    assert len(ghosts)==4 and all(float(g['healthy_A_residual_rms'])<1e-7 for g in ghosts)
    for g in ghosts:
        assert g['feedback']=='0' and abs(float(g['A_gain'])-.34**int(g['pass']))<2e-6
    for p in first.glob('*.wav'):
        with wave.open(str(p)) as w:
            assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(1,2,44100)
            assert w.getnframes()==(441000 if p.name.startswith('1') else 16384+211680)
    assert (first/'04_healthy_matched.wav').read_bytes()!=(first/'05_weak_erase_matched.wav').read_bytes()
    for name in ['00_dry_raw.wav','10_AB_input.wav','settings.json','medium.csv']:
        target=first/name;target.unlink();os.link(source,target)
        run('--input',source,'--output',first,ok=False)
        assert hashlib.sha256(source.read_bytes()).hexdigest()==original;target.unlink()
    for args in [[],['--unknown'],['--input'],['--output','']]:run(*args,ok=False)
    bad=root/'bad.wav';bad.write_bytes(b'not PCM')
    run('--input',bad,'--output',root/'bad-output',ok=False)
    assert not (root/'bad-output').exists()
print('Tape CLI: eleven byte-stable WAVs, level matching, feedback-zero ghost proof and source protection passed')
