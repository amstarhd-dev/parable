"""Evaluate against held-out full-circuit guitar plus a continuous knob sweep."""
from pathlib import Path
import csv
import subprocess
import sys

import numpy as np
from scipy import signal

exe, captures, report = sys.argv[1:]
path=Path(captures)
audio=(path/"test.f32").read_bytes()

def run(s,t,v,f,data,extra=()):
    cmd=[exe,str(s),str(t),str(v),str(f),*extra]
    return np.frombuffer(subprocess.check_output(cmd,input=data),"<f4").astype(float)

def rms(x):
    return np.linalg.norm(x)/np.sqrt(len(x))

def db(ratio):
    return 20*np.log10(max(float(ratio),1e-12))

with open(report,"w",newline="") as file:
    out=csv.writer(file)
    out.writerow(("fault","sustain","tone","volume","normalized_error_rms","relative_level_db","correlation","candidate_peak"))
    for ref in sorted(path.glob("test-*.f32")):
        _,f,s,t,v=ref.stem.split("-")
        reference=np.fromfile(ref,"<f4").astype(float)[1024:]
        result=run(s,t,v,f,audio)[1024:]
        out.writerow([f,s,t,v,rms(reference-result)/rms(reference),db(rms(result)/rms(reference)),np.corrcoef(reference,result)[0,1],max(abs(result))])
        assert len(result)==len(reference) and np.isfinite(result).all()
    n=np.arange(96000)
    stress=(.4*signal.chirp(n/48000,100,2,10000)).astype("<f4").tobytes()
    for f in range(2):
        moving=run(.5,.5,.5,f,stress,("sweep",))
        muted=run(.5,.5,0,f,audio)
        assert np.isfinite(moving).all() and max(abs(moving))<1 and not np.any(muted)
        print(f"fault {f}: sweep peak {max(abs(moving)):.5f}; Volume zero verified")
