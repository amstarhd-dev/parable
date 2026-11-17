"""Native whole-core listening deliverables; independent of subjective quality."""
import hashlib,json,subprocess,sys,tempfile,wave,csv
from pathlib import Path
binary,source=Path(sys.argv[1]).resolve(),Path(sys.argv[2]).resolve()
with tempfile.TemporaryDirectory() as directory:
 output=Path(directory)/'renders';command=[str(binary),'--input',str(source),'--output',str(output)]
 subprocess.run(command,check=True);files=list(output.glob('*.wav'));assert len(files)==25
 hashes={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in files}
 for file in files:
  with wave.open(str(file),'rb') as f:
   assert (f.getnchannels(),f.getsampwidth(),f.getframerate())==(1,2,48000)
   if file.name=='fault_unexcited_zero.wav':assert not any(f.readframes(f.getnframes()))
 assert hashes['healthy_weak.wav']==hashes['fault_weak.wav']
 assert hashes['healthy_raw.wav']!=hashes['fault_raw.wav']
 assert all(int(r['output_clamps'])==0 for r in csv.DictReader((output/'levels.csv').open()))
 subprocess.run(command,check=True);assert hashes=={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in files}
 assert subprocess.run([str(binary),'--bad','argument'],capture_output=True).returncode!=0
print('25 native WAVs, actual loop diagnostics, weak identity, no self-start, unclipped raw levels and fresh repeatability pass')
