#!/usr/bin/env python3
"""SF012-specific reproducible reference and listening packaging, stdlib only."""
import argparse,concurrent.futures,csv,hashlib,json,math,os,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def reference(executable,out):
    executable=executable.resolve();out.mkdir(parents=True,exist_ok=True)
    fixtures=ROOT/'tests/fixtures/tape-saturation';manifest=json.loads((fixtures/'reference-manifest.json').read_text())
    for name,digest in manifest['source_sha256'].items():assert sha(ROOT/name)==digest,name
    for name,digest in manifest['fixture_sha256'].items():assert sha(fixtures/name)==digest,name
    generated=out/'generated-fixtures';subprocess.run([str(executable),'--fixtures',str(generated)],check=True)
    comparisons={}
    for name in manifest['fixture_sha256']:
        old=list(csv.reader((fixtures/name).open()));new=list(csv.reader((generated/name).open()));assert len(old)==len(new) and old[0]==new[0],name
        maximum=0
        for a,b in zip(old[1:],new[1:]):
            assert len(a)==len(b)
            for x,y in zip(a,b):
                try:xf,yf=float(x),float(y)
                except ValueError:assert x==y;continue
                assert math.isfinite(xf) and math.isfinite(yf) and math.isclose(xf,yf,abs_tol=1e-10,rel_tol=1e-11),(name,x,y)
                maximum=max(maximum,abs(xf-yf))
        comparisons[name]=dict(rows=len(old)-1,maximum_numeric_difference=maximum,generated_sha256=sha(generated/name))
    (out/'fixture-reproduction.json').write_text(json.dumps(comparisons,indent=2)+'\n')
    carrier=out/'carrier';carrier.mkdir(exist_ok=True)
    cases=[(kind,amplitude,bias,rate,steps) for kind,amplitude in [('tone',.2),('hf',.8)] for bias in [1,.5] for rate,steps in [(1536000,64),(3072000,128)]]
    def run(case):
        kind,a,b,rate,steps=case;path=carrier/f'{kind}-{a}-{b}-{rate}-{steps}.csv'
        subprocess.run([str(executable),'--carrier',str(path),str(rate),str(steps),kind,str(a),str(b),'offline'],check=True);print(path.name,flush=True)
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:list(pool.map(run,cases))
def package(audio,repeat,source,research):
    for path in audio.rglob('*'):
        if path.is_file() and path.name!='performance.csv':assert path.read_bytes()==(repeat/path.relative_to(audio)).read_bytes(),path
    assert len(list(audio.glob('*.wav')))==57
    expected=json.loads((ROOT/'third_party/tape_saturation/provenance.json').read_text())['audio']
    for name,key in [('source-original-stereo24.wav','original_sha256'),('take03-20s-mono16.wav','prepared_excerpt_sha256')]:
        assert sha(source/name)==expected[key];shutil.copyfile(source/name,audio/name)
    shutil.copyfile(source/'source-receipt.json',audio/'source-receipt.json')
    for directory,dest in [(ROOT/'third_party/tape_saturation','notices'),(ROOT/'tests/fixtures/tape-saturation','independent-reference'),(ROOT/'build/tape-saturation-measurements','host-numerical'),(ROOT/'docs/figures/sf012','figures'),(research,'research')]:shutil.copytree(directory,audio/dest,dirs_exist_ok=True)
    for name in ['SF012_GATE0.md','SF012_REPORT.md','SF012_PRESERVATION.json']:shutil.copyfile(ROOT/'docs'/name,audio/name)
    shutil.copyfile(ROOT/'docs/adr/ADR-012-moving-tape-underbias.md',audio/'ADR-012-moving-tape-underbias.md')
    (audio/'SOURCE_COMMIT').write_text(subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True))
    (audio/'HEAD_COMMIT').write_text(os.environ.get('HEAD_COMMIT','unavailable')+'\n')
    (audio/'HOST_COMPILER.txt').write_text(subprocess.check_output(['c++','--version'],text=True))
    paths=['include/parable_tape_saturation.h','src/parable_tape_saturation.cpp','src/tape_saturation_material.h','tools/lab_wav.h','tools/soundforge/analysis.h','tools/soundforge/excitation.h','tools/klon_input.py','tests/activation_guard.cpp','tests/activation_guard.h']
    for pattern in ['tools/tape_saturation*','tools/check_tape_saturation_arm.py','tests/test_tape_saturation*','third_party/tape_saturation/*','tests/fixtures/tape-saturation/*']:paths.extend(str(p.relative_to(ROOT)) for p in ROOT.glob(pattern) if p.is_file())
    (audio/'SOURCE_SHA256SUMS').write_text(''.join(sha(ROOT/p)+'  '+p+'\n' for p in sorted(set(paths))))
    (audio/'SHA256SUMS').write_text(''.join(sha(p)+'  '+str(p.relative_to(audio))+'\n' for p in sorted(audio.rglob('*')) if p.is_file() and p.name!='SHA256SUMS'))
    print('Verified57 repeated native WAVs;59 including identical original/excerpt; source pins and all artifact checksums recorded')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='mode',required=True)
    r=sub.add_parser('reference');r.add_argument('--reference',type=Path,required=True);r.add_argument('--output',type=Path,required=True)
    a=sub.add_parser('package');a.add_argument('--audio',type=Path,required=True);a.add_argument('--repeat',type=Path,required=True);a.add_argument('--input',type=Path,required=True);a.add_argument('--research',type=Path,required=True)
    args=p.parse_args()
    if args.mode=='reference':reference(args.reference,args.output)
    else:package(args.audio,args.repeat,args.input,args.research)
