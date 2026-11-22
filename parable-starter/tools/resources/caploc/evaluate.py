"""Run with host executable, float32 DI, 48 kHz float32 cab IR, and output folder."""
import csv
from pathlib import Path
import subprocess,sys
import numpy as np
from scipy.signal import fftconvolve
from scipy.io import wavfile
exe,di,cab,out=sys.argv[1:]; out=Path(out);fs=48000
x=np.fromfile(di,'<f4');ir=np.fromfile(cab,'<f4')
def drive(audio,r,f):
 y=np.frombuffer(subprocess.check_output([exe,str(r),str(f)],input=np.asarray(audio,'<f4').tobytes()),'<f4').astype(float)
 assert y.size==len(audio) and np.isfinite(y).all() and max(abs(y))<=1
 return y
def rms(x):return np.sqrt(np.mean(x*x))
def db(x):return 20*np.log10(max(x,1e-15))
def tone(f,d,a):
 t=np.arange(round(fs*d))/fs;return a*np.sin(2*np.pi*f*t)
# Second pick deliberately arrives under the detector's stored charge.
hit=tone(137,.09,.72)*np.exp(-np.arange(4320)/(fs*.05))
probe=tone(220,.19,.015)*np.exp(-np.arange(9120)/(fs*.1))
pair=np.r_[hit,np.zeros(960),probe,np.zeros(4800)]
parts=[x,*[drive(x,r,f) for r,f in [(.42,0),(1,0),(1,1)]]]
cabs=[fftconvolve(a,ir)[:len(a)] for a in parts]
# Match active six-second RMS for timbre; no compression or within-passage leveling.
gains=[1/rms(a[:6*fs]) for a in cabs]
demo=np.concatenate([a*g for a,g in zip(cabs,gains)])
wavfile.write(out/'Capricorn_Level_Loc_V30.wav',fs,np.int16(np.rint(32767*.88*demo/max(abs(demo)))))
rows=[]
for r,f in [(.42,0),(1,0),(1,1)]:
 a=drive(pair,r,f); b=drive(np.r_[np.zeros(5280),probe,np.zeros(4800)],r,f)
 onset=slice(5280+200,5280+1160)
 rows.append(['second note after loud note vs isolated dB',r,f,db(rms(a[onset])/rms(b[onset]))])
 for amp in [.01,.04,.16,.64]:
  y=drive(tone(200,.8,amp),r,f)[-12000:]
  spectrum=abs(np.fft.rfft(y)); fundamental=spectrum[50]; harmonics=spectrum[np.arange(2,16)*50]
  rows.append([f'200Hz input {amp} THD harmonics 2-15 percent',r,f,100*np.linalg.norm(harmonics)/fundamental])
  rows.append([f'200Hz input {amp} RMS dBFS',r,f,db(rms(y))])
 rows.append(['stress peak abs',r,f,max(abs(drive(np.tile([1,-1,np.nan,np.inf,-np.inf,0,0,.1],20000),r,f)))])
with (out/'host_results.csv').open('w') as h:
 w=csv.writer(h);w.writerow(['test','register','fault','value']);w.writerows(rows)
print([row for row in rows if row[0].startswith('second')])
