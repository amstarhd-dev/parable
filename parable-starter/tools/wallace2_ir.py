#!/usr/bin/env python3
"""Prepare full real measurements, no trimming/time alignment/minimum-phase EQ."""
import argparse,hashlib,json,subprocess
from pathlib import Path
import numpy as np
p=argparse.ArgumentParser();p.add_argument('--directory',type=Path,required=True);a=p.parse_args();receipts={}
for name,sha in [('jensen','894805dc8b60bd7e2b49dac582bc27a39c8d1f191fc110eb4d4145c0b63937bb'),('v30','ef8d258eee57b2f0fa1e678b4bdf9c8f535f03039172b0831dcc87b0e7d99bd2')]:
 src=a.directory/(name+'-original.wav');assert hashlib.sha256(src.read_bytes()).hexdigest()==sha
 cmd=['ffmpeg','-v','error','-i',str(src),'-af','aresample=48000:filter_size=64:phase_shift=10:exact_rational=1','-f','f32le','-acodec','pcm_f32le','-']
 x=np.frombuffer(subprocess.check_output(cmd),dtype='<f4').astype(float);spectrum=np.abs(np.fft.rfft(x,262144));gain=1/spectrum.max();h=(x*gain).astype('<f4');(a.directory/(name+'-48k.f32')).write_bytes(h.tobytes())
 trimmed=len(h)
 while trimmed and abs(h[trimmed-1])<1e-6:trimmed-=1
 receipts[name]=dict(original_sha256=sha,kernel_sha256=hashlib.sha256(h.tobytes()).hexdigest(),ffmpeg_argv=cmd,frames=len(h),seconds=len(h)/48000,gain=float(gain),normalization='unity peak magnitude on262144point FFT; common scalar only',first_over_minus60db_peak=int(np.flatnonzero(abs(h)>=max(abs(h))*.001)[0]),peak_sample=int(np.argmax(abs(h))),l1_norm=float(abs(h).sum()),upstream_effective_taps=trimmed,upstream_trim_l1_error=float(abs(h[trimmed:]).sum()))
(a.directory/'preparation.json').write_text(json.dumps(dict(ffmpeg=subprocess.check_output(['ffmpeg','-version'],text=True).splitlines()[0],numpy=np.__version__,irs=receipts),indent=2)+'\n')
print(json.dumps(receipts,indent=2))
