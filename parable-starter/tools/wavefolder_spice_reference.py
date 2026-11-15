#!/usr/bin/env python3
"""SF014 independent builtin-BJT SPICE and high-rate sampling reference.

Offline only. No native model headers/stamps/interpolant or native output is
used to generate expected values. NumPy2.3.5 is a research tool, not a DSP/build
dependency; CI host tests consume authenticated committed references.
"""
import argparse,csv,hashlib,json,math,shutil,subprocess
from pathlib import Path
import numpy as np
from rat_spice_reference import run_spice
ROOT=Path(__file__).resolve().parents[1]
SIZE=4096
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def circuit(rp=15000,load=10000,rail=15):
    return f'''Independent published Lockhart topology, reciprocal generic BJT, ideal buffered Vin
VCC P 0 {rail:.17g}
VEE N 0 {-rail:.17g}
VIN IN 0 0
RP P EP {rp:.17g}
RN EN N 15000
RL C 0 {load:.17g}
QP C IN EP Pmodel
QN C IN EN Nmodel
.model Pmodel PNP(IS=1e-17 BF=100 BR=1 CJE=0 CJC=0 TF=0 TR=0 TNOM=26.85)
.model Nmodel NPN(IS=1e-17 BF=100 BR=1 CJE=0 CJC=0 TF=0 TR=0 TNOM=26.85)
.save all @qp[ic] @qn[ic] @qp[ie] @qn[ie]
'''
def write(path,header,rows):
    with path.open('w',newline='') as f:
        w=csv.writer(f,lineterminator='\n');w.writerow(header);w.writerows(rows)
def excitation(rate,kind):
    # Coherent at-or-below nominal note, matching the declared input definition.
    # Retain actual Hz, never infer a round() bin from its label.
    bin_=int([1760,3520,8000,1000][kind]*SIZE/rate);n=np.arange(SIZE)
    if kind<3:v=np.sin(2*np.pi*bin_*n/SIZE)
    else:
        v=np.zeros(SIZE)
        for b in [76,97,113]:
            for h in range(1,6):v+=np.sin(2*np.pi*b*h*n/SIZE)/(h*h)
    peak=float(max(abs(v)));return (v.astype(np.float32).astype(float)*.2/peak).astype(np.float32).astype(float),bin_
def fir(factor):
    n=np.arange(48*factor+1);u=n-24*factor
    h=np.sinc(2*.4/factor*u)*2*.4/factor*(.42-.5*np.cos(2*np.pi*n/(48*factor))+.08*np.cos(4*np.pi*n/(48*factor)))
    return h/sum(h)
def fft_up(x,length):
    source=np.fft.rfft(x);padded=np.zeros(length//2+1,dtype=complex);padded[:len(source)]=source
    if len(x)%2==0:padded[len(x)//2]*=.5
    return np.fft.irfft(padded,n=length)*length/len(x)
def reference_wave(x,rate,dense,resolution=128,factor=4):
    # Independent mathematical implementation of the specified linear stages.
    # The reference nonlinear law is densely solved SPICE, NOT native Hermite.
    h=fir(factor);z=np.zeros(SIZE*factor);z[::factor]=x;hpad=np.zeros_like(z);hpad[:len(h)]=h;H=np.fft.rfft(hpad)
    internal=factor*np.fft.irfft(np.fft.rfft(z)*H,n=len(z))
    drive=fft_up(internal,SIZE*resolution)*6
    raw=np.interp(drive,dense[:,0],-dense[:,1]);spectrum=np.fft.rfft(raw)/(SIZE*resolution)
    omega=2*np.pi*np.arange(SIZE//2+1)/SIZE;g0=math.tan(math.pi*(1/(2*math.pi*.01))/rate);g=g0/(1+g0);z1=np.exp(-1j*omega);hp=(1-g)*(1-z1)/(1-(1-2*g)*z1)
    target=spectrum[:SIZE//2+1]*H[:SIZE//2+1]*hp/8
    return np.fft.irfft(target*SIZE,n=SIZE),target
def generate(exe,work,out):
    work.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True);netlists=out/'netlists';netlists.mkdir(exist_ok=True)
    (work/'spinit').write_text('set num_threads=1\n');version=subprocess.check_output([str(exe),'--version'],text=True)
    if '44.2' not in version:raise ValueError('Require the pinned ngspice44.2 reference')
    dense={};dc=[];candidates=[];loadrail=[]
    vectors='v(C) v(EP) v(EN) @qp[ic] @qn[ic] @qp[ie] @qn[ie]'
    for rp in [15000,22000,33000]:
        data=np.array(run_spice(str(exe),work,f'dense-RP{rp}',circuit(rp),'set numdgt=17\ndc VIN -3 3 .00005',vectors));assert len(data)==120001 and np.isfinite(data).all();dense[rp]=data
        for row in data[::100]:dc.append([rp,10000,15,*map(float,row)])
        x=data[:,0];y=-data[:,1];slope=np.gradient(y,x);folds=np.flatnonzero(slope[1:]*slope[:-1]<0)+1
        for i in folds:candidates.append([rp,float(x[i]),float(y[i]),float(slope[i-1]),float(slope[i]),float(y[len(y)//2])])
        # Saved branch vectors, not scalar post-analysis @Q values.
        assert max(abs(data[:,1]/10000+data[:,4]+data[:,5]))<2e-9
    for rp in [15000,22000]:
        for load in [1000,5000,10000,50000]:
            for rail in [12,15,18]:
                data=run_spice(str(exe),work,f'load-RP{rp}-RL{load}-rail{rail}',circuit(rp,load,rail),'set numdgt=17\ndc VIN -1.5 1.5 .01',vectors)
                assert len(data)==301
                loadrail.extend([rp,load,rail,*row] for row in data[::10])
    header=['Rplus_ohms','load_ohms','rail_V','input_V','collector_V','EP_V','EN_V','QP_IC_A','QN_IC_A','QP_IE_A','QN_IE_A']
    write(out/'dc.csv',header,dc);write(out/'load-rail.csv',header,loadrail)
    write(out/'candidates.csv',['Rplus_ohms','fold_input_V','readout_at_fold_V','left_slope','right_slope','zero_input_readout_V'],candidates)
    harmonic=[]
    for rp in [15000,22000]:
        for amplitude in [.2,.6,1.2]:
            for bias in [0,.1]:
                n=65536;phase=2*np.pi*np.arange(n)/n;values=np.interp(bias+amplitude*np.sin(phase),dense[rp][:,0],-dense[rp][:,1]);s=np.fft.rfft(values)/n
                harmonic.append([rp,amplitude,bias,float(s[0].real),math.sqrt(float(np.mean(values**2))),*map(float,[2*abs(s[h]) for h in [1,2,3,5,7,9]])])
    write(out/'harmonics.csv',['Rplus_ohms','input_peak_V','input_bias_V','raw_DC_V','raw_RMS_V','H1_V','H2_V','H3_V','H5_V','H7_V','H9_V'],harmonic)
    waves=[];meta=[];refinement=[]
    for rate in [44100,48000,96000]:
        for fault,rp in [(0,15000),(1,22000)]:
            for kind in range(4):
                x,bin_=excitation(rate,kind);y,s=reference_wave(x,rate,dense[rp]);fine,fspec=reference_wave(x,rate,dense[rp],64)
                error=float(np.sqrt(np.mean((y-fine)**2)));assert error<2e-8
                # Spatial/DC grid refinement independent of temporal resolution.
                coarse,_=reference_wave(x,rate,dense[rp][::2]);gridError=float(np.sqrt(np.mean((y-coarse)**2)));assert gridError<2e-8
                meta.append([rate,fault,kind,bin_*rate/SIZE if kind<3 else 0,SIZE,float(np.sqrt(np.mean(y*y))),float(max(abs(y))),bin_])
                refinement.append([rate,fault,kind,error,gridError])
                waves.extend([rate,fault,kind,n,float(v)] for n,v in enumerate(y))
    write(out/'reference.csv',['rate_hz','fault','kind','frame','reference_normalized'],waves)
    write(out/'reference-levels.csv',['rate_hz','fault','kind','actual_fundamental_Hz','frames','reference_RMS','reference_peak','input_bin'],meta)
    write(out/'refinement.csv',['rate_hz','fault','kind','64_to128_RMS_normalized','DC_grid_refinement_RMS_normalized'],refinement)
    # Retain literal circuits/logs, not megabytes of redundant adaptive/raw dumps.
    for path in sorted(work.glob('*.cir')):shutil.copyfile(path,netlists/path.name)
    manifest=dict(simulator=version.strip(),simulator_package_url='https://deb.debian.org/debian/pool/main/n/ngspice/ngspice_44.2+ds-1_amd64.deb',simulator_package_sha256='4d76721dd91416055b1088cfc58626d4db47b91afda7c73258c0968ba6e4d24e',numpy_version=np.__version__,model='Independent literal complementary Lockhart, builtin reciprocal BJT. Static law only; no calibrated device charge/hardware.',native_code_used=False,thermal_note='SPICE300K constants versus modern SI native300K, small reported mismatch; paper rounded25.864mV is not silently substituted.',reference=dict(frames=SIZE,temporal_factors=[64,128],DC_step_V=.00005,coarse_DC_step_V=.0001,Drive=6,source_peak_V=.2,normal_factor=4,ADAA_delay='half internal sample is explicitly aligned during comparison',filter='independently computed specified FIR and output coupling; ideal alias-free reference before base-rate sampling',output_divisor=8),source_sha256={str(Path(__file__).relative_to(ROOT)):sha(Path(__file__)), 'tools/rat_spice_reference.py':sha(ROOT/'tools/rat_spice_reference.py')},fixture_sha256={p.name:sha(p) for p in sorted(out.glob('*.csv'))},netlist_sha256={p.name:sha(p) for p in sorted(netlists.glob('*.cir'))},raw_SPICE_sha256={p.name:sha(p) for p in sorted(work.glob('*.txt'))})
    (out/'spice-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps(dict(DC_rows=len(dc),load_rail_rows=len(loadrail),harmonic_rows=len(harmonic),wave_rows=len(waves),cases=len(meta),refinement_max=max(r[3] for r in refinement),grid_refinement_max=max(r[4] for r in refinement)),indent=2))
def check(existing,out):
    manifest=json.loads((existing/'spice-manifest.json').read_text())
    for name,digest in manifest['fixture_sha256'].items():
        assert sha(existing/name)==digest,name
        old=list(csv.reader((existing/name).open()));new=list(csv.reader((out/name).open()));assert len(old)==len(new) and old[0]==new[0]
        for a,b in zip(old[1:],new[1:]):
            for x,y in zip(a,b):assert math.isclose(float(x),float(y),rel_tol=1e-8,abs_tol=2e-10),(name,x,y)
    print('Independent references replay within1e-8relative/2e-10absolute; committed fixtures were not overwritten')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ngspice',type=Path,required=True);p.add_argument('--work',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--check-existing',type=Path);a=p.parse_args()
    generate(a.ngspice.resolve(),a.work.resolve(),a.output.resolve())
    if a.check_existing:check(a.check_existing,a.output)
