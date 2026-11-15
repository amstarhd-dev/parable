#!/usr/bin/env python3
"""Package a native exact-repeat listening run with source/reference receipts."""
import argparse,hashlib,json,os,shutil,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--renders',required=True,type=Path);p.add_argument('--repeat',required=True,type=Path);p.add_argument('--input',required=True,type=Path);p.add_argument('--tests',required=True,type=Path);a=p.parse_args();out=a.renders
for f in out.iterdir():
    if f.is_file() and f.name!='performance.csv':assert f.read_bytes()==(a.repeat/f.name).read_bytes(),f
assert len(list(out.glob('*.wav')))==67
fixtures=root/'tests/fixtures/wavefolder';m=json.loads((fixtures/'spice-manifest.json').read_text())
for key,base in [('fixture_sha256',fixtures),('netlist_sha256',fixtures/'netlists'),('source_sha256',root)]:
    for name,digest in m[key].items():assert hashlib.sha256((base/name).read_bytes()).hexdigest()==digest,name
for name in ['source-original-stereo24.wav','take03-20s-mono16.wav','source-receipt.json']:shutil.copy2(a.input/name,out/name)
for f in (root/'third_party/wavefolder').iterdir():shutil.copy2(f,out/f.name)
for name in ['SF014_GATE0.md','SF014_REPORT.md','adr/ADR-014-lockhart-feed-resistor.md']:shutil.copy2(root/'docs'/name,out/Path(name).name)
shutil.copy2(a.tests,out/'HOST_TESTS.txt');shutil.copytree(fixtures,out/'independent-reference',dirs_exist_ok=True);shutil.copytree(root/'docs/figures/sf014',out/'figures',dirs_exist_ok=True)
shutil.copytree(root/'docs/data/sf014',out/'selection-study',dirs_exist_ok=True)
(out/'SOURCE_COMMIT').write_text(subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True));(out/'HEAD_COMMIT').write_text(os.environ.get('HEAD_COMMIT',subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip())+'\n');(out/'HOST_COMPILER.txt').write_text(subprocess.check_output(['c++','--version'],text=True))
paths=[]
for pattern in ['include/parable_wavefolder.h','src/parable_wavefolder.cpp','src/wavefolder_cell.h','tools/wavefolder*','tools/check_wavefolder_arm.py','tests/test_wavefolder*','tests/activation_guard.*','tools/lab_wav.h','tools/rat_spice_reference.py','tools/klon_input.py','tools/soundforge/analysis.h','tools/soundforge/excitation.h','tools/soundforge/registry.json','third_party/wavefolder/*','tests/fixtures/wavefolder/*','tests/fixtures/wavefolder/netlists/*','include/parable_rat.h','src/parable_rat.cpp','include/parable_muff.h','src/parable_muff.cpp','src/muff_device.h','include/parable_cmos.h','src/parable_cmos.cpp','src/cmos_device.h']:
    paths.extend(f for f in root.glob(pattern) if f.is_file())
(out/'SOURCE_SHA256SUMS').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+str(f.relative_to(root))+'\n' for f in sorted(set(paths))))
(out/'SHA256SUMS').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+str(f.relative_to(out))+'\n' for f in sorted(out.rglob('*')) if f.is_file() and f.name!='SHA256SUMS'))
print('Exact repeat verified;69 WAVs with source originals, attributed transforms, references and complete checksums')
