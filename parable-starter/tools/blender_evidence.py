#!/usr/bin/env python3
"""Verify actual native WAV receipts, preserve same-take credit and source identity.

This packages evidence, never turns failed science into approval. All numerical
failures and unperformed target/human work remain explicit in the manifest.
"""
import argparse
import csv
import gzip
import hashlib
import json
import math
from pathlib import Path
import shutil
import struct
import subprocess
import wave

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--renders", type=Path, required=True)
p.add_argument("--input", type=Path, required=True)
p.add_argument("--identity", type=Path)
p.add_argument("--arm", type=Path)
p.add_argument("--tests", type=Path, action="append", default=[])
p.add_argument("--native", type=Path)
p.add_argument("--study", type=Path)
a = p.parse_args()
out = a.renders.resolve()


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


assert sha(a.input / "take03-20s-mono16.wav") == "75ae5e73c72e6774fb75e6318dc91ba66347fb65290949e947144033eb648f52"
assert sha(a.input / "source-original-stereo24.wav") == "84980b5468f3315171e4e546a1d38d54e4c368ccb5fee72c759e0915f346fad0"
settings = json.loads((out / "settings.json").read_text())
assert not settings["quick_plumbing_only"] and settings["supplied_PCM16"] and settings["deterministic_second_pass"] and not settings["founder_approved"]
rows = list(csv.DictReader((out / "renders.csv").open()))
assert len(rows) == 81
audition_names = {row["file"] for row in rows}
assert len(audition_names) == 81 and all(Path(n).name == n for n in audition_names)
assert {p.name for p in out.glob("*.wav")} in [audition_names, audition_names | {"source-original-stereo24.wav", "take03-20s-mono16.wav"}]
common, matched = {}, {}
for row in rows:
    path = out / row["file"]
    with wave.open(str(path), "rb") as w:
        assert w.getparams()[:3] == (1, 2, int(row["rate_hz"])) and w.getnframes() == int(row["frames"])
        count, square, total, peak, clips = 0, 0., 0., 0., 0
        while True:
            b = w.readframes(8192)
            if not b:
                break
            for (value,) in struct.iter_unpack("<h", b):
                v = value / 32768
                count += 1
                square += v * v
                total += v
                peak = max(peak, abs(v))
                clips += value in [-32768, 32767]
    actual = [math.sqrt(square / count), peak, total / count]
    assert all(abs(x - float(row[k])) < 2e-10 for x, k in zip(actual, ["PCM_rms", "PCM_peak", "PCM_DC"]))
    assert all(abs(x - float(row[k])) <= 1 / 65536 + 1e-8 for x, k in zip(actual, ["final_float_rms", "final_float_peak", "final_float_DC"]))
    assert not clips and peak < .999 and 0 < float(row["offline_gain"]) <= 16.00001
    assert all(int(row[k]) == 0 for k in ["numerical_failures", "input_clamps", "output_clamps", "PCM_clips"])
    group = row["file"].split("_external_dry_")[0].split("_healthy_")[0].split("_D1_open_")[0]
    if row["file"].endswith("_common.wav"):
        common.setdefault(group, []).append(float(row["offline_gain"]))
    if row["file"].endswith("_rms_matched.wav") and row["file"].startswith("input_"):
        matched.setdefault(group, []).append(float(row["final_float_rms"]))
assert len(common) == 9 and all(len(v) == 3 and max(v) == min(v) for v in common.values())
assert len(matched) == 5 and all(len(v) == 3 and max(v) - min(v) < 1e-7 for v in matched.values())
provenance = out / "provenance"
provenance.mkdir(exist_ok=True)
for name in ["source-original-stereo24.wav", "take03-20s-mono16.wav"]:
    shutil.copy(a.input / name, out / name)
shutil.copy(a.input / "source-receipt.json", provenance / "guitar-source.json")
for name in ["AUDIO_NOTICE.md", "ACME-LICENSE.md", "provenance.json"]:
    shutil.copy(ROOT / "third_party/blender" / name, provenance / name)
for name in ["SF016_GATE0.md", "SF016_REPORT.md", "SF016_WORK_LOG.md", "adr/ADR-016-original-blender.md"]:
    shutil.copy(ROOT / "docs" / name, provenance / Path(name).name)
if a.identity:
    identity = json.loads(a.identity.read_text())
    assert all(sha(ROOT / name) == digest for name, digest in identity["components"].items()), "Producing components changed after render"
    shutil.copy(a.identity, provenance / "render-identity.json")
else:
    # CI runs this immediately after building/rendering the checked-out source.
    identity = dict(source_commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(), components={name: sha(ROOT / name) for name in ["include/parable_blender.h", "src/parable_blender.cpp", "src/blender_device.h", "tools/blender_lab.cpp"]})
    (provenance / "render-identity.json").write_text(json.dumps(identity, indent=2) + "\n")
for test in a.tests:
    assert test.is_file()
    shutil.copy(test, provenance / test.name)
if a.arm:
    shutil.copytree(a.arm, provenance / "arm", dirs_exist_ok=True)
if a.native:
    shutil.copy(a.native, provenance / "native-physical.f32")
if a.study:
    shutil.copytree(a.study, provenance / "sampling-study", dirs_exist_ok=True)
shutil.copytree(ROOT / "docs/data/sf016", provenance / "reported-data", dirs_exist_ok=True)
shutil.copytree(ROOT / "tests/fixtures/blender", provenance / "independent-reference", dirs_exist_ok=True)
if (ROOT / "docs/figures/sf016").is_dir():
    shutil.copytree(ROOT / "docs/figures/sf016", provenance / "figures", dirs_exist_ok=True)
# Preserve large numerical traces exactly with bounded repository blob size.
state = out / "state.csv"
if state.exists():
    original = dict(file="state.csv", bytes=state.stat().st_size, sha256=sha(state))
    compressed = out / "state.csv.gz"
    with state.open("rb") as source, compressed.open("wb") as target:
        with gzip.GzipFile(filename="", mode="wb", fileobj=target, compresslevel=6, mtime=0) as z:
            shutil.copyfileobj(source, z)
    with gzip.open(compressed, "rb") as source:
        h = hashlib.sha256()
        for block in iter(lambda: source.read(1024 * 1024), b""):
            h.update(block)
    assert h.hexdigest() == original["sha256"]
    (provenance / "state-compression.json").write_text(json.dumps(original, indent=2) + "\n")
    state.unlink()
readme = """# SF016 listening — DRAFT research, not accepted DSP

Start input_sustain_mid_b100_boost0_{external_dry,healthy,D1_open}_common.wav,
then the healthy/D1_open RMS-matched pair. Raw/common retain actual fault level.
Five same-take control settings, four explicitly synthetic probes and matched
main RAT/Muff/CMOS/Aquarius comparisons are separate.81 native audition WAVs,
plus exact original stereo24 and prepared first20s mono16 Guitar-TECHS take03.
Full CC-BY4 attribution/change notice is in provenance/AUDIO_NOTICE.md.

Only physical D1 opens. All native render pairs repeat bit-exactly before writing
and reject numerical containment. Actual float/PCM RMS/peak/DC/gains are in
renders.csv. Raw=jackVolts/8 with modeled coupling and8x FIR, not unfiltered DC.
RMS matching is not LUFS or subjective approval. The internal Blend dry tap is
preamp-derived, different from the included external DI.

Scientific gates FAILED:21 field checks in two8kHz transient cases; only28/48
combined sampling gates pass, with14/48 fine reference convergence unresolved.
Coarse1x/2x modes have rejected solver failures; no such failures in delivered
8x auditions. This source is standalone and not in Engine/firmware. No founder
listening approval, physical pedal calibration or Daisy runtime/stack/cycle proof.
See source commit/component hashes and all retained strict failures in provenance.
state.csv.gz preserves every numerical trace byte; its original hash is recorded.
"""
(out / "README.md").write_text(readme)
files = {str(path.relative_to(out)): sha(path) for path in sorted(out.rglob("*")) if path.is_file() and path.name not in ["MANIFEST.json", "SHA256SUMS"]}
manifest = dict(experimental=True, merge_approval=False, source=identity, native_audition_WAVs=81, total_WAVs=83, verified_actual_PCM_levels=True, exact_second_native_pass=True, founder_approved=False, physical_hardware_tested=False, scientific_acceptance="FAILED; see retained physical/sampling gates", checksum_algorithm="SHA256", files=files)
(out / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
with (out / "SHA256SUMS").open("w") as f:
    for path in sorted(out.rglob("*")):
        if path.is_file() and path.name != "SHA256SUMS":
            f.write(sha(path) + "  " + str(path.relative_to(out)) + "\n")
print("Verified81 native WAVs/83 with sources, actual levels, exact producing identity; science FAILED")
