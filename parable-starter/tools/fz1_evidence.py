#!/usr/bin/env python3
"""Package actual native listening, independently regenerated fixtures and receipts."""
import argparse,csv,hashlib,json,os,shutil,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--renders',type=Path,required=True);p.add_argument('--input',type=Path,required=True);p.add_argument('--tests',type=Path,required=True);p.add_argument('--arm',type=Path);p.add_argument('--netlists',type=Path,action='append',default=[]);a=p.parse_args();out=a.renders
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
expected={'source-original-stereo24.wav':'84980b5468f3315171e4e546a1d38d54e4c368ccb5fee72c759e0915f346fad0','take03-20s-mono16.wav':'75ae5e73c72e6774fb75e6318dc91ba66347fb65290949e947144033eb648f52'}
for name,digest in expected.items():assert sha(a.input/name)==digest,name
assert len(list(out.glob('*.wav'))) in (102,104)
r=list(csv.DictReader((out/'renders.csv').open()));assert len(r)==102
cost=list(csv.DictReader((out/'performance.csv').open()));assert len(cost)==25 and all(row[k]=='0' for row in cost for k in ['failures','input_clamps','output_clamps'])
settings=json.loads((out/'settings.json').read_text());assert settings['normal_factor']==8 and settings['FIR_taps']==385
fixtures=root/'tests/fixtures/fz1'
for name,source in [('spice-manifest.json','fz1_spice_reference.py'),('equation-manifest.json','fz1_equation_reference.py')]:
 m=json.loads((fixtures/name).read_text())
 for file,digest in m['fixtures'].items():assert sha(fixtures/file)==digest,file
 assert m['source_sha256']==sha(root/'tools'/source),source
for name in [*expected,'source-receipt.json']:shutil.copy2(a.input/name,out/name)
for f in (root/'third_party/fz1').iterdir():shutil.copy2(f,out/f.name)
for name in ['ACME-LICENSE.md','WHITEHEAD-LICENSE.md']:shutil.copy2(root/'third_party/germanium'/name,out/name)
for name in ['SF015_GATE0.md','SF015_REPORT.md','SF015_WORK_LOG.md','adr/ADR-015-original-fz1.md']:shutil.copy2(root/'docs'/name,out/Path(name).name)
shutil.copy2(a.tests,out/'HOST_TESTS.txt');shutil.copytree(fixtures,out/'independent-reference',dirs_exist_ok=True);shutil.copytree(root/'docs/figures/sf015',out/'figures',dirs_exist_ok=True);shutil.copytree(root/'docs/data/sf015',out/'measurements',dirs_exist_ok=True)
for directory in a.netlists:
 target=out/'netlists'/directory.name;target.mkdir(parents=True,exist_ok=True)
 for pattern in ['*.cir','*.log','spinit']:
  for f in directory.glob(pattern):shutil.copy2(f,target/f.name)
if a.arm:shutil.copytree(a.arm,out/'arm-object-audit',dirs_exist_ok=True)
(out/'SOURCE_COMMIT').write_text(subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True));(out/'SOURCE_WORKTREE_DIRTY').write_text(str(bool(subprocess.check_output(['git','status','--porcelain'],cwd=root,text=True)))+'\n');(out/'HOST_COMPILER.txt').write_text(subprocess.check_output(['c++','--version'],text=True))
paths=[]
for pattern in ['include/parable_fz1.h','src/parable_fz1.cpp','src/germanium_device.h','include/parable_germanium.h','tools/fz1*','tools/check_fz1_arm.py','tests/test_fz1*','tests/activation_guard.*','tools/lab_wav.h','tools/klon_input.py','tools/soundforge/analysis.h','tools/soundforge/excitation.h','tools/soundforge/registry.json','include/parable_rat.h','src/parable_rat.cpp','include/parable_muff.h','src/parable_muff.cpp','src/muff_device.h','src/parable_germanium.cpp','include/parable_cmos.h','src/parable_cmos.cpp','src/cmos_device.h','third_party/fz1/*']:
 paths.extend(f for f in root.glob(pattern) if f.is_file())
(out/'SOURCE_SHA256SUMS').write_text(''.join(sha(f)+'  '+str(f.relative_to(root))+'\n' for f in sorted(set(paths))))
(out/'SHA256SUMS').write_text(''.join(sha(f)+'  '+str(f.relative_to(out))+'\n' for f in sorted(out.rglob('*')) if f.is_file() and f.name!='SHA256SUMS'))
print('104 WAVs including pinned originals; native exact reset repeat occurred inside each render; source/reference/level/ARM receipts packaged. No human or hardware approval.')
