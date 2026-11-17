"""Focused candidate research: fresh-process WAV repeatability and CLI boundaries.
This does not assert chamber quality, full reset correctness or embedded safety.
"""
import hashlib,json,subprocess,sys,tempfile,wave
from pathlib import Path
binary,source=Path(sys.argv[1]).resolve(),Path(sys.argv[2]).resolve()
with tempfile.TemporaryDirectory() as directory:
 output=Path(directory)/'renders';command=[str(binary),str(source),str(output),'compact']
 subprocess.run(command,check=True)
 files=list(output.glob('*.wav'));assert len(files)==5
 hashes={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in files}
 receipt=json.loads((output/'compact_receipt.json').read_text());assert receipt['callback_guard_clean']
 assert receipt['raw_rms']>0 and receipt['raw_peak']<1 and receipt['matched_target_rms']<=.04
 for file in files:
  with wave.open(str(file),'rb') as w:assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(1,2,48000)
 subprocess.run(command,check=True)
 assert hashes=={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in files}
 assert subprocess.run([str(binary),str(source),str(output),'unknown'],capture_output=True).returncode!=0
 assert subprocess.run([str(binary)],capture_output=True).returncode!=0
print('Fresh-process reproducibility, PCM and callback research checks passed; no admission implied')
