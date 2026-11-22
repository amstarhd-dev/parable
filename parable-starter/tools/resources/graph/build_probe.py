"""Build the reduced model against the repo's pinned libDaisy/toolchain."""
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
from pathlib import Path

here = Path(__file__).resolve().parent
project = here.parents[2]
sys.path.insert(0, str(project / "firmware"))
from build import PINS, archive, git

cache = project / "build/ts808-probe-cache"
cache.mkdir(exist_ok=True, parents=True)
toolbin = archive(cache, PINS["toolchain"], "toolchain.tar.gz") / "bin"
sdk = cache / "libDaisy"
pin = PINS["libdaisy"]
def cmd(*args):
    subprocess.run(list(map(str,args)), check=True)
if not sdk.exists():
    cmd("git","init",sdk)
    cmd("git","-C",sdk,"remote","add","origin",pin["url"])
cmd("git","-C",sdk,"fetch","--depth","1","origin",pin["commit"])
cmd("git","-C",sdk,"checkout","--detach",pin["commit"])
cmd("git","-C",sdk,"submodule","update","--init","--depth","1","--",*pin["submodules"])
assert all(git(sdk / name,"rev-parse","HEAD")==rev for name,rev in pin["submodules"].items())
cmd("make","-C",sdk,"-j2",f"GCC_PATH={toolbin}")
names=sorted(p.name for p in here.iterdir() if p.suffix in (".cpp",".h",".py") or p.name=="Makefile")
hashes={n:hashlib.sha256((here/n).read_bytes()).hexdigest() for n in names}
identity=hashlib.sha256(json.dumps([hashes,PINS],sort_keys=True).encode()).hexdigest()
(here/"probe_id.h").write_text('#define PROBE_ID "'+identity+'"\n')
dst=project/"build/graph-kit"
dst.mkdir(exist_ok=True)
import subprocess
args=["make","-C",here,"-j2",f"GCC_PATH={toolbin}",f"LIBDAISY_DIR={sdk}"]
try:
    cmd(*args)
    layout="BOOT_NONE"
except subprocess.CalledProcessError:
    cmd(*args,"clean")
    cmd(*args,"APP_TYPE=BOOT_SRAM")
    layout="BOOT_SRAM"
binary=here/"build/patch_graph.bin"
data=binary.read_bytes()
sp,reset=struct.unpack_from("<II",data)
assert reset&1
if layout=="BOOT_NONE":assert len(data)<=131072 and 0x08000000<=reset<0x08000000+len(data)
else:assert 0x24000000<=reset<0x24000000+len(data)
shutil.copy(binary,dst)
for f in sdk.rglob("*.bin"):
    if "intdfu" in f.name.lower() and "2000" in f.name:shutil.copy(f,dst/"bootloader.bin");break
(dst/"manifest.json").write_text(json.dumps(dict(id=identity,sha256=hashlib.sha256(data).hexdigest(),layout=layout,sources=hashes),indent=2))
