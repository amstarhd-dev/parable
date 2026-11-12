"""Actual rotary runner: repeat audio, levels, motion and input protection."""
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
import xml.etree.ElementTree as ET


def run(*args,ok=True):
    r=subprocess.run([sys.argv[1],*map(str,args)],text=True,capture_output=True)
    assert (r.returncode==0)==ok,r.stdout+r.stderr


with tempfile.TemporaryDirectory(prefix='rotary-cli-') as directory:
    root=Path(directory);source=root/'source.wav';frames=6*44100
    with wave.open(str(source),'wb') as w:
        w.setparams((1,2,44100,0,'NONE','not compressed'))
        w.writeframes(struct.pack('<'+str(frames)+'h',*[round(4000*(math.sin(2*math.pi*997*i/44100)+.5*math.sin(2*math.pi*97*i/44100))) for i in range(frames)]))
    digest=hashlib.sha256(source.read_bytes()).hexdigest();one,two=root/'one',root/'two'
    for output in (one,two):run('--input',source,'--output',output)
    for p in one.iterdir():
        if p.name!='renders.csv':assert p.read_bytes()==(two/p.name).read_bytes(),p.name
    rows=list(csv.DictReader((one/'renders.csv').open()));assert len(rows)==12
    assert max(float(r['final_rms']) for r in rows[3:6])-min(float(r['final_rms']) for r in rows[3:6])<1e-8
    assert abs(float(rows[6]['final_rms'])-float(rows[7]['final_rms']))<1e-8
    assert int(rows[1]['horn_releases'])==0 and int(rows[2]['horn_releases'])>=5 and int(rows[2]['horn_captures'])>=5
    assert all(float(r['final_peak'])<.9 for r in rows)
    for p in one.glob('*.wav'):
        with wave.open(str(p)) as w:
            assert w.getnchannels()==1 and w.getsampwidth()==2
            assert (w.getframerate(),w.getnframes())==((48000,384000) if p.name.startswith(('09','10','11')) else (44100,352800))
    ET.parse(one/'motion.svg')
    comparison=list(csv.DictReader((one/'comparison.csv').open()))[0]
    assert comparison['drum_sample_exact']=='true' and comparison['containment_events']=='0'
    assert float(comparison['matched_difference_rms'])>.001
    assert json.loads((one/'settings.json').read_text())['delay_cells_per_band']==384
    alias=list(csv.DictReader((one/'alias.csv').open()))
    assert len(alias)==2 and all(math.isfinite(float(r['reference_out_of_band_db_total'])) for r in alias)
    assert float(alias[0]['reference_power_above_native_nyquist_fraction'])<1e-6
    assert float(alias[1]['reference_power_above_native_nyquist_fraction'])>.01  # limitation exposed, not alias-free.
    for name in ['00_dry_raw.wav','04_healthy_matched.wav','motion.csv','settings.json']:
        p=one/name;p.unlink();os.link(source,p);run('--input',source,'--output',one,ok=False)
        assert hashlib.sha256(source.read_bytes()).hexdigest()==digest;p.unlink()
    for args in [[],['--input'],['--unknown'],['--output','']]:run(*args,ok=False)
    bad=root/'bad.wav';bad.write_bytes(b'not PCM');run('--input',bad,'--output',root/'bad-output',ok=False)
    assert not (root/'bad-output').exists()
print('Rotary CLI: 12 repeatable WAVs, raw/matched levels, motion SVG, mechanical discrimination and source protection passed')
