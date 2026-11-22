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
names=["probe.cpp","capricorn_loc.h","Makefile","build_probe.py"]
hashes={n:hashlib.sha256((here/n).read_bytes()).hexdigest() for n in names}
identity=hashlib.sha256(json.dumps([hashes,PINS],sort_keys=True).encode()).hexdigest()
(here/"probe_id.h").write_text('#define PROBE_ID "'+identity+'"\n')
cmd("make","-C",here,"-j2",f"GCC_PATH={toolbin}",f"LIBDAISY_DIR={sdk}")
binary=here/"build/capricorn_loc.bin"
data=binary.read_bytes()
sp,reset=struct.unpack_from("<II",data)
assert 8<len(data)<=131072 and 0x20000000<sp<=0x20020000 and reset&1 and 0x08000000<=reset<0x08000000+len(data)
dst=project/"build/caploc-kit"
dst.mkdir(exist_ok=True)
shutil.copy(binary,dst)
for n in ("run_probe.py","RUN_CAPRICORN.command"):
    shutil.copy(here/n,dst)
for n in ("seed3_runner.py","seed3_oneboot_batch.py","hardware.py"):
    shutil.copy(here.parent/n,dst)
(dst/"manifest.json").write_text(json.dumps(dict(id=identity,sha256=hashlib.sha256(data).hexdigest(),sources=hashes),indent=2))
