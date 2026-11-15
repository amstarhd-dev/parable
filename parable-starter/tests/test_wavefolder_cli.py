"""Actual waveform/levels/metadata and strict source protection; standard library."""
import csv,hashlib,json,math,os,struct,subprocess,sys,tempfile,wave
from pathlib import Path
# Authenticate independently generated data AND the exact offline generator.
project=Path(__file__).resolve().parents[1];fixtures=project/'tests/fixtures/wavefolder'
manifest=json.loads((fixtures/'spice-manifest.json').read_text())
for key,base in [('fixture_sha256',fixtures),('netlist_sha256',fixtures/'netlists'),('source_sha256',project)]:
    for name,digest in manifest[key].items():assert hashlib.sha256((base/name).read_bytes()).hexdigest()==digest,name
def run(*args,ok=True):
    p=subprocess.run([sys.argv[1],*map(str,args)],capture_output=True,text=True)
    assert (p.returncode==0)==ok,p.stdout+p.stderr
with tempfile.TemporaryDirectory(prefix='fold-cli-') as t:
    root=Path(t);source=root/'source.wav';rate=44100
    data=[round(6000*math.sin(2*math.pi*196*n/rate)*math.exp(-8*((n/rate)%.25))) for n in range(rate)]
    with wave.open(str(source),'wb') as w:w.setparams((1,2,rate,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+str(rate)+'h',*data))
    digest=hashlib.sha256(source.read_bytes()).hexdigest();out=root/'out';run('--input',source,'--output',out,'--compare-existing')
    rows=list(csv.DictReader((out/'renders.csv').open()));by={r['file']:r for r in rows};assert len(rows)==len(by)==67
    assert all(r[k]=='0' for r in rows for k in ['numerical_failures','input_clamps','table_clamps','output_clamps'])
    assert all(float(r['final_peak'])<.999 and 0<float(r['offline_gain'])<=16.00001 for r in rows)
    assert all(float(r['offline_gain'])==1 for r in rows if r['file'].endswith('_raw.wav'))
    for label,drive in [('restrained',1),('medium',3),('intense',6)]:
        matched=[by[f'{label}_{kind}_rms_matched.wav'] for kind in ['dry','healthy','RP22k']]
        assert max(float(r['final_rms']) for r in matched)-min(float(r['final_rms']) for r in matched)<1e-8
        h=by[f'{label}_healthy_raw.wav'];f=by[f'{label}_RP22k_raw.wav'];assert float(h['condition_drive'])==float(f['condition_drive'])==drive
        assert h['input_gain_V_per_source_unit']==f['input_gain_V_per_source_unit'] and h['upper_feed_ohms']=='15000' and f['upper_feed_ohms']=='22000'
        assert (out/h['file']).read_bytes()!=(out/f['file']).read_bytes()
    for group in ['restrained','medium','intense','attacks','sustain','chord','sweep','two']:
        assert len({by[f'{group}_{k}_common.wav']['offline_gain'] for k in ['dry','healthy','RP22k']})==1
    for p in out.glob('*.wav'):
        with wave.open(str(p)) as w:assert w.getnchannels()==1 and w.getsampwidth()==2 and w.getnframes()>w.getframerate()/2
    states=list(csv.DictReader((out/'state.csv').open()));assert states and all(abs(float(r['interpolated_KCL_peak_A']))<2e-7 for r in states)
    assert any(r['fault']=='1' and abs(float(r['instant_readout_V']))>.1 for r in states)
    for name in ['restrained_healthy_raw.wav','intense_RP22k_rms_matched.wav','compare_RAT_raw.wav','state.csv','settings.json','LISTEN.txt']:
        p=out/name;p.unlink();os.link(source,p);run('--input',source,'--output',out,ok=False);assert hashlib.sha256(source.read_bytes()).hexdigest()==digest;p.unlink()
    for args in [[],['--input'],['--unknown'],['--output',''],['--output',out,'--output',root/'two']]:run(*args,ok=False)
    bad=root/'bad.wav';bad.write_bytes(b'bad');run('--input',bad,'--output',root/'bad-out',ok=False);assert not (root/'bad-out').exists()
print('67 native WAVs, gain/fault pairs, common/RMS/raw levels, branch KCL, metadata and source safety passed')
