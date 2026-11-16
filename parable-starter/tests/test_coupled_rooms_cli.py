"""Focused actual native WAV boundary/repeatability test; no external downloads."""
import hashlib
import math
import struct
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

binary = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    source = root/'source.wav'
    with wave.open(str(source), 'wb') as f:
        f.setparams((1, 2, 48000, 0, 'NONE', 'not compressed'))
        f.writeframes(b''.join(struct.pack('<h', round(6000*math.sin(2*math.pi*220*n/48000))) for n in range(12000)))
    output = root/'output'
    command = [str(binary), '--input', str(source), '--output', str(output)]
    subprocess.run(command, check=True)
    files = sorted(output.glob('*.wav'))
    assert len(files) == 19, [f.name for f in files]
    hashes = {f.name: hashlib.sha256(f.read_bytes()).hexdigest() for f in files}
    for path in files:
        with wave.open(str(path), 'rb') as f:
            assert (f.getnchannels(), f.getsampwidth(), f.getframerate()) == (1, 2, 48000)
            data = f.readframes(f.getnframes())
            if path.name == 'coupling_off_B.wav':
                assert not any(data)
            else:
                assert any(data), path
    assert hashes['healthy_raw.wav'] != hashes['fault_raw.wav']
    subprocess.run(command, check=True)
    assert hashes == {f.name: hashlib.sha256(f.read_bytes()).hexdigest() for f in files}
    assert subprocess.run([str(binary), '--unknown', 'x'], capture_output=True).returncode != 0
    assert subprocess.run([str(binary), '--input'], capture_output=True).returncode != 0
    source.write_bytes(b'bad input')
    assert subprocess.run(command, capture_output=True).returncode != 0
print('Native PCM outputs, no-coupling silence, repeatability and rejected CLI inputs pass')
