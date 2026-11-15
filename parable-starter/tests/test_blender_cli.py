"""Native PCM/level/identity/boundary tests. Quick renders are plumbing only."""
import csv
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave

EXE = str(Path(sys.argv[1]).resolve())


def run(*args, ok=True):
    p = subprocess.run([EXE, *map(str, args)], capture_output=True, text=True, timeout=240)
    assert (p.returncode == 0) == ok, (args, p.returncode, p.stdout, p.stderr)


def wav(path, rate=48000, silent=False, channels=1):
    with wave.open(str(path), "wb") as w:
        w.setparams((channels, 2, rate, 0, "NONE", "not compressed"))
        w.writeframes(b"".join(struct.pack("<h", 0 if silent else round(4000 * math.sin(2 * math.pi * 196 * n / rate))) * channels for n in range(rate // 10)))


def stats(samples):
    return math.sqrt(sum(x * x for x in samples) / len(samples)), max(map(abs, samples)), sum(samples) / len(samples)


def verify(out, rate):
    settings = json.loads((out / "settings.json").read_text())
    assert settings["quick_plumbing_only"] and settings["deterministic_second_pass"]
    assert not settings["founder_approved"] and settings["only_fault"] == "D1_open"
    rows = list(csv.DictReader((out / "renders.csv").open()))
    assert len(rows) == 45 and len(list(out.glob("*.wav"))) == 45
    common = {}
    matched = {}
    for row in rows:
        path = out / row["file"]
        with wave.open(str(path), "rb") as w:
            assert w.getparams()[:3] == (1, 2, rate)
            assert w.getnframes() == int(row["frames"])
            samples = [x / 32768 for x in struct.unpack("<" + "h" * w.getnframes(), w.readframes(w.getnframes()))]
        measured = stats(samples)
        assert all(abs(x - float(row[k])) < 2e-12 for x, k in zip(measured, ["PCM_rms", "PCM_peak", "PCM_DC"]))
        assert all(abs(x - float(row[k])) <= 1 / 65536 + 1e-8 for x, k in zip(measured, ["final_float_rms", "final_float_peak", "final_float_DC"]))
        gain = float(row["offline_gain"])
        assert 0 < gain <= 16.00001 and measured[1] < .999
        assert abs(float(row["final_float_rms"]) - gain * float(row["raw_rms"])) < 1e-7
        assert all(int(row[k]) == 0 for k in ["numerical_failures", "input_clamps", "output_clamps", "PCM_clips"])
        group = row["file"].split("_external_dry_")[0].split("_healthy_")[0].split("_D1_open_")[0]
        if row["file"].endswith("_common.wav"):
            common.setdefault(group, []).append(gain)
        if row["file"].endswith("_rms_matched.wav"):
            matched.setdefault(group, []).append(float(row["final_float_rms"]))
    assert len(common) == len(matched) == 5
    assert all(len(v) == 3 and max(v) == min(v) for v in common.values())
    assert all(len(v) == 3 and max(v) - min(v) < 1e-7 for v in matched.values())
    return {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in out.glob("*.wav")}


with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    original = {}
    for rate in [44100, 48000, 96000]:
        source = root / f"source-{rate}.wav"
        wav(source, rate)
        before = source.read_bytes()
        out = root / f"render-{rate}"
        run("--input", source, "--output", out, "--quick")
        original[rate] = verify(out, rate)
        assert source.read_bytes() == before
    second = root / "second"
    run("--input", root / "source-48000.wav", "--output", second, "--quick")
    assert verify(second, 48000) == original[48000]
    for args in [[], ["--output"], ["--bogus", "x"], ["--output", root / "x", "--output", root / "y"], ["--output", root / "x", "--quick", "--compare-existing"]]:
        run(*args, ok=False)
    for name, kwargs in [("silence", {"silent": True}), ("stereo", {"channels": 2}), ("rate", {"rate": 32000})]:
        path = root / (name + ".wav")
        wav(path, **kwargs)
        destination = root / (name + "-rejected")
        run("--input", path, "--output", destination, "--quick", ok=False)
        assert not destination.exists()
    invalid = root / "broken.wav"
    invalid.write_bytes(b"RIFF")
    run("--input", invalid, "--output", root / "broken-out", ok=False)
    # Direct identity and hard-link alias protections must run before any output.
    aliasdir = root / "alias"
    aliasdir.mkdir()
    path = aliasdir / "input_sustain_low_b100_boost0_external_dry_raw.wav"
    wav(path)
    before = path.read_bytes()
    run("--input", path, "--output", aliasdir, "--quick", ok=False)
    assert path.read_bytes() == before and len(list(aliasdir.iterdir())) == 1
    linked = root / "linked"
    linked.mkdir()
    (linked / path.name).hardlink_to(path)
    run("--input", path, "--output", linked, "--quick", ok=False)
    assert path.read_bytes() == before and len(list(linked.iterdir())) == 1
print("SF016 native CLI: all rates, 45-file levels, actual PCM, repeat and boundaries passed")
