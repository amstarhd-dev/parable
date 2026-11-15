"""Ten native listening files, honest levels, immutable input and pinned fixtures."""
import csv, hashlib, json, math, struct, subprocess, sys, tempfile, wave
from pathlib import Path

root = Path(__file__).resolve().parents[1]
fixtures = root / 'tests/fixtures/scorpio'
for name, digest in json.loads((fixtures/'reference.json').read_text())['sha256'].items():
    assert hashlib.sha256((fixtures/name).read_bytes()).hexdigest() == digest, name

def run(*args, ok=True):
    result = subprocess.run([sys.argv[1], *map(str, args)], capture_output=True, text=True)
    assert (result.returncode == 0) == ok, result.stdout + result.stderr

with tempfile.TemporaryDirectory(prefix='scorpio-cli-') as temporary:
    directory = Path(temporary)
    source = directory/'input.wav'
    rate = 44100
    values = [round(6000*math.sin(2*math.pi*196*n/rate)*math.exp(-8*((n/rate)%.25))) for n in range(rate)]
    with wave.open(str(source), 'wb') as stream:
        stream.setparams((1, 2, rate, 0, 'NONE', 'not compressed'))
        stream.writeframes(struct.pack('<'+str(rate)+'h', *values))
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    out = directory/'audio'
    run('--input', source, '--output', out)
    rows = list(csv.DictReader((out/'renders.csv').open()))
    by_name = {r['file']: r for r in rows}
    assert len(rows) == len(by_name) == len(list(out.glob('*.wav'))) == 10
    for row in rows:
        assert all(row[k] == '0' for k in ['failures', 'table_clamps', 'clipped_samples'])
        assert 0 < float(row['offline_gain']) <= 16.00001
        with wave.open(str(out/row['file'])) as stream:
            assert stream.getnchannels() == 1 and stream.getsampwidth() == 2
            samples = [v[0]/32768 for v in struct.iter_unpack('<h', stream.readframes(stream.getnframes()))]
        assert abs(math.sqrt(sum(x*x for x in samples)/len(samples))-float(row['final_rms'])) < 1/32768
        assert abs(max(map(abs, samples))-float(row['final_peak'])) < 1/32768
    for group, gain in [('low', 0), ('high', 1)]:
        for kind in ['raw', 'matched']:
            healthy = by_name[f'{group}_healthy_{kind}.wav']
            fault = by_name[f'{group}_R3_10k_{kind}.wav']
            assert healthy['level'] == fault['level'] and float(healthy['gain']) == float(fault['gain']) == gain
            assert (out/healthy['file']).read_bytes() != (out/fault['file']).read_bytes()
            if kind == 'raw':
                assert healthy['offline_gain'] == fault['offline_gain'] == '1'
            else:
                assert abs(float(healthy['final_rms'])-float(fault['final_rms'])) < 1e-8
    assert float(by_name['low_healthy_raw.wav']['raw_rms']) < float(by_name['high_healthy_raw.wav']['raw_rms'])
    # Existing destination (including hard-linked source) is rejected before writing.
    protected = out/'high_healthy_raw.wav'
    protected.unlink()
    protected.hardlink_to(source)
    run('--input', source, '--output', out, ok=False)
    assert hashlib.sha256(source.read_bytes()).hexdigest() == digest
    for args in [[], ['--unknown'], ['--input', source, '--output', source]]:
        run(*args, ok=False)
print('10 native WAVs, raw/matched levels, gain/fault pairs, PCM receipts, fixture integrity and source protection passed')
