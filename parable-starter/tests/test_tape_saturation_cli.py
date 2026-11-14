"""Actual native musical groups, one bias fault, gain receipts and source safety."""
import csv,hashlib,json,math,os,struct,subprocess,sys,tempfile,wave
from pathlib import Path
def run(*args,ok=True):
    result=subprocess.run([sys.argv[1],*map(str,args)],capture_output=True,text=True)
    assert (result.returncode==0)==ok,result.stdout+result.stderr
with tempfile.TemporaryDirectory(prefix='sf012-cli-') as temporary:
    root=Path(temporary);source=root/'source.wav';rate=44100
    values=[round(6000*(1-math.exp(-(n/rate% .22)*3000))*math.exp(-(n/rate% .22)*12)*math.sin(2*math.pi*196*n/rate)) for n in range(rate)]
    with wave.open(str(source),'wb') as w:
        w.setparams((1,2,rate,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+str(len(values))+'h',*values))
    digest=hashlib.sha256(source.read_bytes()).hexdigest();out=root/'audio';run('--input',source,'--output',out)
    rows=list(csv.DictReader((out/'renders.csv').open()));by={r['file']:r for r in rows};assert len(rows)==len(by)==57
    assert all(r['numerical_failures']==r['input_clamps']==r['output_clamps']=='0' for r in rows)
    assert all(0<float(r['offline_gain'])<=16.000001 and float(r['final_peak'])<.999 for r in rows)
    assert all(float(r['offline_gain'])==1 for r in rows if r['file'].endswith('_raw.wav'))
    for group in ['low','medium','hard']:
        matched=[by[f'{group}_{signal}_rms_matched.wav'] for signal in ['dry','healthy','weakbias']]
        assert max(float(r['final_rms']) for r in matched)-min(float(r['final_rms']) for r in matched)<1e-8
        raw=[by[f'{group}_{signal}_raw.wav'] for signal in ['dry','healthy','weakbias']]
        assert len(set(r['input_gain_V_per_source_unit'] for r in raw))==1
        assert float(raw[1]['bias_peak_A'])==.012 and float(raw[2]['bias_peak_A'])==.006
        assert (out/raw[1]['file']).read_bytes()!=(out/raw[2]['file']).read_bytes()
        assert all(float(r['raw_rms'])>1e-5 for r in raw)
    for group in ['low','medium','hard','attacks','sustain','chord','two','sweep']:
        assert len(set(by[f'{group}_{signal}_common.wav']['offline_gain'] for signal in ['dry','healthy','weakbias']))==1
    for p in out.glob('*.wav'):
        with wave.open(str(p)) as w:
            assert (w.getnchannels(),w.getsampwidth())==(1,2)
            expected=(rate,rate*23//20) if p.name.startswith(('low_','medium_','hard_')) else (48000,103200 if p.name.startswith('sustain_') else 31200 if p.name.startswith('attacks_') else 64800)
            assert (w.getframerate(),w.getnframes())==expected,p.name
    settings=json.loads((out/'settings.json').read_text());assert settings['healthy_bias_peak_A']==.012 and settings['weak_bias_peak_A']==.006 and settings['bias_Hz']==100000
    assert settings['oversampling']==4 and settings['state_bytes']<=16384 and settings['offline_gain_ceiling']==16
    performance=list(csv.DictReader((out/'performance.csv').open()));assert len(performance)==16
    assert all(int(r['internal_parcels'])==4*int(r['frames']) and 0<float(r['us_per_frame'])<100000 for r in performance)
    state=list(csv.DictReader((out/'state.csv').open()));assert state and all(abs(float(r['last4x_M']))<=1 and math.isfinite(float(r['coil_backward_difference_V'])) for r in state)
    trace=list(csv.DictReader((out/'parcels.csv').open()));assert trace and all(-.000211<float(r['relative_s'])<=0 and abs(float(r['M']))<=1.000001 for r in trace)
    assert len(list(csv.DictReader((out/'histograms.csv').open())))==16*128
    # A hardlink at any prospective output must be rejected BEFORE a write.
    for name in ['medium_healthy_raw.wav','hard_weakbias_rms_matched.wav','renders.csv','parcels.csv','settings.json','LISTEN.txt']:
        p=out/name;p.unlink();os.link(source,p);run('--input',source,'--output',out,ok=False);assert hashlib.sha256(source.read_bytes()).hexdigest()==digest;p.unlink()
    for args in [[],['--input'],['--unknown'],['--output',''],['--output',out,'--output',root/'two']]:run(*args,ok=False)
    bad=root/'bad.wav';bad.write_bytes(b'invalid');run('--input',bad,'--output',root/'bad-output',ok=False);assert not (root/'bad-output').exists()
print('SF01257 native WAVs, three identical-source calibrations, one bias-current fault, raw/common/RMS receipts and source safety')
