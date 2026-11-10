"""Real runner/PCM16 regression; test-owned excitation, not a DSP reference."""
import csv
import hashlib
import json
import os
import struct
import subprocess
import sys
import tempfile
import wave
from pathlib import Path


def run(executable, *args, success=True):
    result = subprocess.run([executable, *map(str,args)], capture_output=True, text=True)
    if (result.returncode == 0) != success:
        raise AssertionError(result.stdout+result.stderr)


def main(executable):
    with tempfile.TemporaryDirectory(prefix='germanium-cli-') as directory:
        root=Path(directory)
        source=root/'owned-test.wav'
        samples=[0]*1024
        samples[64]=64
        with wave.open(str(source),'wb') as stream:
            stream.setparams((1,2,44100,0,'NONE','not compressed'))
            stream.writeframes(struct.pack('<1024h',*samples))
        original=source.read_bytes()
        first,second=root/'first',root/'second'
        for output in (first,second):
            run(executable,'--input',source,'--output',output)
        assert source.read_bytes()==original
        assert sorted(p.name for p in first.iterdir())==sorted(p.name for p in second.iterdir())
        for path in first.iterdir():
            assert path.read_bytes()==(second/path.name).read_bytes(),path.name
        files=sorted(first.glob('*.wav'))
        assert len(files)==20
        for path in files:
            with wave.open(str(path),'rb') as stream:
                assert (stream.getnchannels(),stream.getsampwidth(),stream.getframerate(),stream.getnframes())==(1,2,44100,1024)
        config=json.loads((first/'configuration.json').read_text())
        assert config['source_kind']=='user-provided-unverified' and config['supply_volts']==-9
        with (first/'renders.csv').open() as stream:
            rows=list(csv.DictReader(line for line in stream if not line.startswith('#')))
        assert len(rows)==10 and all(int(row['failed_solves'])==0 and int(row['contained_outputs'])==0 for row in rows)
        assert len(set(row['relative_common_gain'] for row in rows))==1
        assert all(int(row['solver_calls'])==4096 for row in rows[1:])
        for destination in (first/'matched_00_dry.wav',first/'relative_08_q1_leaky.wav',first/'configuration.json'):
            destination.unlink();os.link(source,destination)
            run(executable,'--input',source,'--output',first,success=False)
            assert source.read_bytes()==original
            destination.unlink()
        run(executable,'--input',second/'matched_00_dry.wav','--output',second,success=False)
        malformed=root/'bad.wav';malformed.write_bytes(b'not PCM16')
        run(executable,'--input',malformed,'--output',root/'bad-output',success=False)
        assert not (root/'bad-output').exists()
        run(executable,'--unknown',success=False)
        run(executable,'--input',success=False)
        run(executable,'--output','',success=False)
        assert hashlib.sha256(source.read_bytes()).digest()==hashlib.sha256(original).digest()
    print('Ge CLI:20 raw/matched WAVs, deterministic PCM16, metadata, counters, invalid input and overwrite guards passed')


if __name__=='__main__':
    main(sys.argv[1])
