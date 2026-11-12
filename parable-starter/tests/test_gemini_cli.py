"""Actual runner: deterministic listening bytes, matching, provenance and source protection."""
import csv
import hashlib
import math
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave


def run(exe, *args, ok=True):
    result = subprocess.run([exe, *map(str, args)], capture_output=True, text=True)
    assert (result.returncode == 0) == ok, result.stdout+result.stderr


with tempfile.TemporaryDirectory(prefix='gemini-cli-') as directory:
    root = Path(directory)
    source = root/'source.wav'
    with wave.open(str(source), 'wb') as w:
        w.setparams((1, 2, 44100, 0, 'NONE', 'not compressed'))
        w.writeframes(struct.pack('<8192h', *[round(4000*math.sin(2*math.pi*997*n/44100)) for n in range(8192)]))
    original = hashlib.sha256(source.read_bytes()).hexdigest()
    first, second = root/'one', root/'two'
    for out in (first, second):
        run(sys.argv[1], '--input', source, '--output', out)
    for p in first.iterdir():
        if p.name != 'renders.csv':  # Measured wall time intentionally varies.
            assert p.read_bytes() == (second/p.name).read_bytes(), p.name
    rows = list(csv.DictReader((first/'renders.csv').open()))
    assert len(rows) == 6
    matched = [float(r['final_rms']) for r in rows[3:]]
    assert max(matched)-min(matched) < 1e-8
    assert all(float(r['final_peak']) <= .890001 for r in rows)
    assert all(r['failed_solves'] == '0' for r in rows)
    assert rows[2]['second_bias_ohms'] == '220000'
    assert rows[1]['second_bias_siemens'] == '0'
    assert rows[1]['second_bias_ohms'] == 'open'
    for p in first.glob('*.wav'):
        with wave.open(str(p)) as w:
            assert (w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()) == (1, 2, 44100, 8192+44100)
    for name in ['00_calibrated_dry.wav', 'healthy.patch.json', 'bias-leak.patch.json', 'renders.csv']:
        target = first/name
        target.unlink()
        os.link(source, target)
        run(sys.argv[1], '--input', source, '--output', first, ok=False)
        assert hashlib.sha256(source.read_bytes()).hexdigest() == original
        target.unlink()
    for args in [[], ['--unknown'], ['--input'], ['--output', '']]:
        run(sys.argv[1], *args, ok=False)
    bad = root/'bad.wav'
    bad.write_bytes(b'not PCM')
    run(sys.argv[1], '--input', bad, '--output', root/'bad-output', ok=False)
    assert not (root/'bad-output').exists()
    assert hashlib.sha256(source.read_bytes()).hexdigest() == original
print('Gemini CLI: six deterministic WAVs, matched RMS, component receipts and overwrite protection passed')
