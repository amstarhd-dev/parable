#!/usr/bin/env python3
"""Package ONLY the RAM identity instrument and its independently reviewed host test."""
import argparse,hashlib,json,re,sys,zipfile
from pathlib import Path
HERE=Path(__file__).resolve().parent
RES=HERE.parent
sys.path.insert(0,str(RES))
import ram_identity_test as ram
import seed3_runner as common

def package(build,output,sdk):
 manifest_hash=common.sha((build/'manifest.json').read_bytes())
 manifest,blob=ram.validate_kit(build/'manifest.json',manifest_hash)
 common.require(common.sha((build/'ram-identity.elf').read_bytes())==manifest['elf_sha256'],'ELF hash mismatch')
 files={p.name:p.read_bytes() for p in build.iterdir() if p.is_file()}
 for name in ['ram_identity_test.py','seed3_runner.py','hardware.py']:files[name]=(RES/name).read_bytes()
 for p in HERE.iterdir():
  if p.is_file():files['source/'+p.name]=p.read_bytes()
 guide=(RES.parents[1]/'docs/resources/RAM_IDENTITY.md').read_text()
 guide=re.sub(r'--manifest-sha256 [0-9a-f]{64}', '--manifest-sha256 '+manifest_hash, guide)
 files['README.md']=guide.encode()
 for name,path in [('libDaisy-LICENSE',sdk/'LICENSE'),('CMSIS-Core-LICENSE',sdk/'Drivers/CMSIS_5/LICENSE.txt'),('CMSIS-Device-LICENSE',sdk/'Drivers/CMSIS-Device/ST/STM32H7xx/LICENSE.md')]:files['licenses/'+name]=path.read_bytes()
 host_hash=common.sha(files['ram_identity_test.py'])
 files['host-manifest.json']=common.json_bytes(dict(schema='parable.ram-host.v1',host_sha256=host_hash,firmware_manifest_sha256=manifest_hash,dependencies={n:common.sha(files[n]) for n in ['seed3_runner.py','hardware.py']},flash_programming_permitted=False))
 files['SHA256SUMS']=''.join(common.sha(data)+'  '+name+'\n' for name,data in sorted(files.items())).encode()
 output.mkdir(parents=True,exist_ok=True)
 filename='seed3-ram-identity-v0.1-'+manifest['artifact_id'][:12]+'-'+host_hash[:12]+'.zip'
 with zipfile.ZipFile(output/filename,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
  for name,data in sorted(files.items()):
   info=zipfile.ZipInfo(name,(2026,11,21,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;archive.writestr(info,data)
 with zipfile.ZipFile(output/filename) as archive:
  common.require(archive.testzip() is None,'Archive CRC failure')
  for name,data in files.items():common.require(archive.read(name)==data,'Archive payload mismatch')
 receipt=dict(schema='parable.ram-identity-kit.v1',file=filename,sha256=common.sha((output/filename).read_bytes()),bytes=(output/filename).stat().st_size,manifest_sha256=manifest_hash,artifact_id=manifest['artifact_id'],host_sha256=host_hash,flash_programming_permitted=False,hardware_tested=False)
 (output/'kit.json').write_bytes(common.json_bytes(receipt));print(json.dumps(receipt,indent=2))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--libdaisy',type=Path,required=True);a=p.parse_args();package(a.build.resolve(),a.output.resolve(),a.libdaisy.resolve())
