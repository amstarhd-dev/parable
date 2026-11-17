#!/usr/bin/env python3
"""Offline SF021 scientific summaries (NumPy2.3.5); no audio/DSP generation."""
import argparse,hashlib,json,wave
from pathlib import Path
import numpy as np
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--renders',type=Path,required=True);p.add_argument('--candidates',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
def wav(path):
 with wave.open(str(path))as w:
  assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(1,2,48000)
  return np.frombuffer(w.readframes(w.getnframes()),'<i2').astype(float)/32768
def impulse(x):
 energy=np.cumsum(x[::-1]**2)[::-1];db=10*np.log10(np.maximum(energy,1e-40)/energy[0]);cross=lambda level:float(np.flatnonzero(db<level)[0])/48000
 density={}
 for ms in [20,50,100,200,500]:
  v=x[int(ms*.001*48000):int((ms*.001+.02)*48000)];rms=np.sqrt(np.mean(v*v));density[str(ms)]=float(np.mean(np.abs(v)>rms)/.3173105)if rms else 0.
 # Spectral high/low energy is descriptive, not a calibrated architectural fit.
 spectral={}
 for start in [.02,.4]:
  v=x[int(start*48000):int((start+.1)*48000)];bins=np.abs(np.fft.rfft(v*np.hanning(len(v)),8192))**2;hz=np.fft.rfftfreq(8192,1/48000);lo=bins[(hz>=100)&(hz<3000)].sum();hi=bins[(hz>=3000)&(hz<12000)].sum();spectral[str(start)]=float(10*np.log10(max(hi,1e-30)/max(lo,1e-30)))
 return dict(t5=cross(-5),t25=cross(-25),T20_extrapolated_RT60=3*(cross(-25)-cross(-5)),first_nonzero_ms=float(np.flatnonzero(x)[0])/48,normalized_echo_density_20ms_windows=density,HF_3k_12k_over_100_3k_dB=spectral)
result=dict(numpy=np.__version__,limits='Numerical descriptors only; no measured chamber IR, professional quality or founder approval inferred. Short-window echo density is threshold-normalized by Gaussian probability; not a perceptual quality score.',candidates={},selected={},probes={})
for engine in ['cloud','signalsmith']:
 path=a.candidates/engine/'envelope_impulse.f32';result['candidates'][engine]=impulse(np.fromfile(path,'<f4').astype(float))
result['selected']['healthy_impulse']=impulse(np.fromfile(a.renders/'healthy_impulse.f32','<f4').astype(float))
for name in ['healthy_weak','fault_weak','healthy_strong','fault_strong','healthy_chord','fault_chord','fault_unexcited_zero','fault_strong_then_disconnect']:
 x=wav(a.renders/(name+'.wav'));late=x[-96000:];n=65536;spectrum=np.abs(np.fft.rfft(late[-n:]*np.hanning(n)));k=int(np.argmax(spectrum[1:])+1);power=float(np.sum(spectrum*spectrum));result['probes'][name]=dict(late_2s_rms=float(np.sqrt(np.mean(late*late))),peak=float(np.max(np.abs(x))),dominant_hz=k*48000/n if power else None,dominant_7bin_power_fraction=float(np.sum(spectrum[max(0,k-3):k+4]**2)/power)if power else 0)
result['weak_wavs_identical']=(a.renders/'healthy_weak.wav').read_bytes()==(a.renders/'fault_weak.wav').read_bytes()
result['source_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
