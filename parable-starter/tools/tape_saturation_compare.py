#!/usr/bin/env python3
"""Stdlib-only resolved-carrier/baseband comparison; no native golden generation.

Native streams use the ACTUAL material kernel via --measure. FFT comparisons use
a declared ideal20k measurement band, NOT a claimed production playback filter.
All continuous-carrier and intermediate native CSVs are retained in artifacts.
"""
import argparse,cmath,csv,hashlib,json,math
from pathlib import Path
def fft(x):
    n=len(x)
    if n==1:return x
    radix=next((r for r in range(2,int(math.sqrt(n))+1) if n%r==0),n)
    parts=[fft(x[r::radix]) for r in range(radix)];stride=n//radix
    roots=[cmath.exp(-2j*math.pi*k/n) for k in range(n)]
    return [sum(parts[r][k%stride]*roots[(r*k)%n] for r in range(radix)) for k in range(n)]
def transform(values):return [v/len(values) for v in fft(list(values))]
def metrics(a,b):
    # Both captures last20ms,50Hz/bin. ExcludeDC from audio error (the actual
    # pickup chain also rejectsDC). Include harmonics AND carrier-induced spurs.
    d=[a[k]-b[k] for k in range(401)];rms=math.sqrt(2*sum(abs(x)**2 for x in d[1:]));reference=math.sqrt(2*sum(abs(x)**2 for x in a[1:401]))
    peak=max(abs(2*sum((d[k]*cmath.exp(2j*math.pi*k*j/2048)).real for k in range(1,401))) for j in range(2048))
    refpeak=max(abs(2*sum((a[k]*cmath.exp(2j*math.pi*k*j/2048)).real for k in range(1,401))) for j in range(2048))
    return dict(audio_RMS_error=rms,audio_peak_error=peak,reference_audio_RMS=reference,reference_audio_peak=refpeak,reference_DC=a[0].real,native_DC=b[0].real)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--carrier',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    carrier={};digests={}
    for path in sorted(a.carrier.glob('*.csv')):
        kind,amplitude,bias,rate,steps=path.stem.split('-');rows=list(csv.DictReader(path.open()));values=[float(r['full_carrier_M']) for r in rows];assert len(values)==int(rate)//50
        carrier[kind,int(float(bias)==.5),int(rate)]=transform(values);digests[path.name]=hashlib.sha256(path.read_bytes()).hexdigest()
    native={}
    for row in csv.DictReader((a.native/'native-material.csv').open()):
        key=(row['kind'],int(row['weak']),int(row['base_rate_hz']));native.setdefault(key,[]).append(float(row['M']))
    result=[];spectra=[]
    for (kind,weak,rate),values in native.items():
        actual=transform(values);reference=carrier[kind,weak,3072000];m=metrics(reference,actual);harsh=kind=='hf';m.update(kind=kind,weak=weak,base_rate_hz=rate)
        m['RMS_gate']=(.005 if harsh else .001)+(.05 if harsh else .03)*m['reference_audio_RMS'];m['peak_gate']=(.02 if harsh else .003)+.05*m['reference_audio_peak']
        m['passed']=m['audio_RMS_error']<m['RMS_gate'] and m['audio_peak_error']<m['peak_gate'];result.append(m)
        for k in range(401):spectra.append(dict(kind=kind,weak=weak,base_rate_hz=rate,hz=k*50,reference_amplitude=abs(reference[k])*(1 if k==0 else 2),native_amplitude=abs(actual[k])*(1 if k==0 else 2)))
    refinement=[]
    for kind in ['tone','hf']:
        for weak in [0,1]:refinement.append(dict(kind=kind,weak=weak,**metrics(carrier[kind,weak,3072000],carrier[kind,weak,1536000])))
    summary=dict(measurement_band_Hz=[50,20000],capture_s=.02,comparison='material remanence, driver tone before record EQ; ideal diagnostic lowpass, no DC; native actual4x kernel versus resolved continuously phased carrier',accuracy=result,refinement=refinement,carrier_sha256=digests,native_sha256=hashlib.sha256((a.native/'native-material.csv').read_bytes()).hexdigest(),physical_validation='shared-assumption numerical agreement; no tape-deck measurements',limitations=['Only coherent1k/.2V and10k/.8V full continuous-carrier probes; separate32phase long-double fixtures cover attacks/chords/twotone/DC/low-hard at all rates.','Combined output-rate/trajectory refinement; not proof of arbitrary sub-audio intermodulation or infinite-band alias freedom.'])
    (a.output/'carrier-comparison.json').write_text(json.dumps(summary,indent=2)+'\n')
    with (a.output/'carrier-spectra.csv').open('w',newline='') as f:
        w=csv.DictWriter(f,list(spectra[0]));w.writeheader();w.writerows(spectra)
    print(json.dumps(summary,indent=2));assert len(result)==12 and all(r['passed'] for r in result)
if __name__=='__main__':main()
