import hashlib
import json
import sys
import time
from pathlib import Path
from seed3_oneboot_batch import Device
from seed3_runner import atomic, require, Stop

def run():
    root=Path(__file__).resolve().parent
    manifest=json.loads((root/"manifest.json").read_text())
    binary=root/"ts808_reduced_480.bin"
    data=binary.read_bytes()
    require(8<len(data)<=131072 and hashlib.sha256(data).hexdigest()==manifest["sha256"],"Firmware checksum/size mismatch")
    out=root/("measurement-"+time.strftime("%Y%m%d-%H%M%S"))
    out.mkdir()
    backend=Device(Path.home()/"Parable-PAR009/runner84-v11/darwin_amd64/dfu-util",out,"200364500000")
    if sys.argv[1:]!=["--read-only"]:
        require(not sys.argv[1:],"Only supported option is --read-only")
        print("Looking for your Daisy in ROM DFU. If needed, hold BOOT, tap RESET, then release BOOT.",flush=True)
        selected=backend.wait_dfu(45)
        backend.flash_selected(selected,binary)
    print("Waiting for the reduced TS808 callback measurement…",flush=True)
    port=backend.open_cdc(60)
    try:
        end=time.monotonic()+15
        while True:
            line=port.line(end)
            if line.startswith("TS808_PROBE_V1 "): break
    finally:
        port.close()
    atomic(out/"report.txt",line.encode("ascii"))
    print(line.strip())
    fields=dict(pair.split("=",1) for pair in line.strip().split()[1:])
    require(fields["id"]==manifest["id"],"Unexpected firmware report")
    expected=dict(seed3=1,core_hz=480000000,rate=48000,counter=1,blocks=3000,bad_frames=0)
    require(all(int(fields[k])==v for k,v in expected.items()),"Clock, sample rate, block size or counter failed verification")
    peak=int(fields["peak_cycles"])
    require(peak>0,"Invalid zero-cycle report")
    print("Peak: %d cycles / 480000 = %.2f%% of a 1 ms block" % (peak,100*peak/480000))
    print("Deadline hits: %s   Nonfinite outputs: %s" % (fields["deadline_hits"],fields["nonfinite"]))
    print("70%% whole-chain timing screen: %s" % ("within" if peak<=336000 and int(fields["nonfinite"])==0 and int(fields["deadline_hits"])==0 else "outside"))
    print("Saved:",out/"report.txt")
if __name__=="__main__":
    try:
        run()
    except (Stop,ValueError,KeyError,OSError) as error:
        sys.exit("STOP: "+str(error))
