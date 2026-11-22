import argparse
import hashlib
import json
from pathlib import Path
import sys
import time

from seed3_oneboot_batch import Device
from seed3_runner import atomic, require, Stop

MODES = ("healthy", "r15")


def next_report(port, deadline, identity):
    while True:
        line = port.line(deadline)
        if line.startswith("MUFF_CONTROL "):
            fields = dict(item.split("=", 1) for item in line.split()[1:])
            require(fields["id"] == identity, "Unexpected firmware identity")
            return line, fields


def screen(fields):
    expected = dict(seed3=1, core_hz=480000000, rate=48000, counter=1)
    require(all(int(fields[k]) == v for k, v in expected.items()), "Board, clock, rate or cycle counter mismatch")
    passed = True
    for mode in MODES:
        blocks, peak, hits, nonfinite, frames = map(int, fields[mode].split(","))
        good = blocks == 3000 and 0 < peak < 96000 and hits == nonfinite == frames == 0
        print(f"{mode}: {peak} peak cycles ({peak/4800:.2f}%) | {blocks} blocks | "
              f"{hits} deadline hits | {nonfinite} nonfinite | {frames} bad frames | "
              f"{'PASS' if good else 'FAIL'}")
        passed &= good
    return passed


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument("--read-only", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    manifest = json.loads((root / "manifest.json").read_text())
    binary = root / "muff_controls.bin"
    data = binary.read_bytes()
    require(8 < len(data) <= 131072 and hashlib.sha256(data).hexdigest() == manifest["sha256"],
            "Firmware checksum or size mismatch")
    out = root / ("measurement-" + time.strftime("%Y%m%d-%H%M%S") + "-" + str(time.time_ns()%1000000000))
    out.mkdir()
    backend = Device(Path.home() / "Parable-PAR009/runner84-v11/darwin_amd64/dfu-util", out, "200364500000")
    if not args.read_only:
        print("Looking for Daisy ROM DFU. Hold BOOT, tap RESET, then release BOOT if needed.", flush=True)
        backend.flash_selected(backend.wait_dfu(45), binary)
    print("Reading the two-mode Muff measurement…", flush=True)
    port = backend.open_cdc(60)
    try:
        line, fields = next_report(port, time.monotonic() + 15, manifest["id"])
        atomic(out / "report.txt", line.encode("ascii"))
        print("Saved", out / "report.txt")
        require(screen(fields), "Muff missed its 20% screen or capture checks")
    finally:
        port.close()


if __name__ == "__main__":
    try:
        run()
    except (Stop, ValueError, KeyError, OSError) as error:
        sys.exit("STOP: " + str(error))
