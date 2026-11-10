"""Exercise the real runner and shared PCM16 I/O; no generated DSP reference."""
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
    result = subprocess.run([executable, *map(str, args)], capture_output=True, text=True)
    if (result.returncode == 0) != success:
        raise AssertionError(result.stdout + result.stderr)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main(executable):
    with tempfile.TemporaryDirectory(prefix='cmos-cli-') as directory:
        root = Path(directory)
        source = root / 'owned-test.wav'
        # Test-generated silence with one quiet impulse, not a real guitar recording.
        samples = [0] * 1024
        samples[64] = 64
        with wave.open(str(source), 'wb') as stream:
            stream.setparams((1, 2, 44100, 0, 'NONE', 'not compressed'))
            stream.writeframes(struct.pack('<1024h', *samples))
        original = digest(source)
        first, second = root / 'first', root / 'second'
        for output in (first, second):
            run(executable, '--input', source, '--output', output)
        assert digest(source) == original
        assert sorted(p.name for p in first.iterdir()) == sorted(p.name for p in second.iterdir())
        for path in first.iterdir():
            assert path.read_bytes() == (second / path.name).read_bytes(), path.name
        files = sorted(first.glob('*.wav'))
        assert len(files) == 8
        for path in files:
            with wave.open(str(path), 'rb') as stream:
                assert (stream.getnchannels(), stream.getsampwidth(), stream.getframerate(), stream.getnframes()) == (1, 2, 44100, 1024)
        config = json.loads((first / 'configuration.json').read_text())
        assert config['source_kind'] == 'user-provided-unverified' and config['sample_rate_hz'] == 44100
        # Both path equality and a differently named hard link must reject before writes.
        for destination in (first / '00_dry.wav', first / 'configuration.json'):
            destination.unlink()
            os.link(source, destination)
            run(executable, '--input', source, '--output', first, success=False)
            assert digest(source) == original
            destination.unlink()
        run(executable, '--input', source, '--output', root, success=True)
        run(executable, '--input', root / '00_dry.wav', '--output', root, success=False)
        assert digest(source) == original
        malformed = root / 'bad.wav'
        malformed.write_bytes(b'not a RIFF PCM16 file')
        run(executable, '--input', malformed, '--output', root / 'bad-output', success=False)
        assert not (root / 'bad-output').exists()
        run(executable, '--unknown', success=False)
        run(executable, '--input', success=False)
        run(executable, '--output', '', success=False)
    print('CMOS CLI: deterministic PCM16, metadata, malformed input and overwrite protection passed')


if __name__ == '__main__':
    main(sys.argv[1])
