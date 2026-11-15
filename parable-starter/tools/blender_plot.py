#!/usr/bin/env python3
"""Source-backed scientific figures; NumPy2.3.5/Matplotlib3.10.8 offline only."""
import argparse
import csv
import json
from pathlib import Path
import struct
import wave
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--renders", type=Path, required=True)
p.add_argument("--native", type=Path, required=True)
p.add_argument("--output", type=Path, required=True)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
plt.rcParams.update({"figure.dpi": 120, "font.size": 10})


def records(path):
    b = path.read_bytes()
    assert b[:8] == b"SF016F32"
    rows, columns = struct.unpack_from("<II", b, 8)
    assert columns == 50 and len(b) == 16 + rows * 208
    return np.frombuffer(b, dtype=np.dtype([("case", "<u4"), ("frame", "<u4"), ("v", "<f4", (50,))]), offset=16)


expected = records(ROOT / "tests/fixtures/blender/transient.f32")
actual = records(a.native)
assert np.array_equal(expected["case"], actual["case"]) and np.array_equal(expected["frame"], actual["frame"])
fig, axes = plt.subplots(3, 2, figsize=(13, 9))
for col, case in enumerate([0, 20]):
    mask = (expected["case"] == case) & (expected["frame"] >= 17300)
    t = expected["frame"][mask] / 192
    e, n = expected["v"][mask], actual["v"][mask]
    for field, label in [(10, "Q3 collector"), (11, "Q3 emitter")]:
        axes[0, col].plot(t, n[:, field] - np.mean(n[:, field]), label=label)
    axes[0, col].set_title(("Healthy" if col == 0 else "Only D1 open") + ":110Hz,.05V, scientific192k direct")
    axes[0, col].set_ylabel("phase-split AC volts")
    axes[0, col].legend()
    for field, label in [(46, "D1"), (47, "D2")]:
        axes[1, col].plot(t, n[:, field] * 1e6, label="native " + label)
        axes[1, col].plot(t, e[:, field] * 1e6, ":", label="SPICE " + label)
    axes[1, col].set_ylabel("intrinsic current µA\n(conduction+diffusion)")
    axes[1, col].legend(ncol=2)
    axes[2, col].plot(t, n[:, 30], label="native jack")
    axes[2, col].plot(t, e[:, 30], ":", label="independent SPICE")
    axes[2, col].set_ylabel("jack volts, no matching")
    axes[2, col].set_xlabel("time ms")
    axes[2, col].legend()
fig.suptitle("Actual loaded transistor split → diode recombination; no ideal x/−x rectifier\nIntrinsic currents exclude the declared800k/package Cj; listening uses48k/8x, not this direct validation grid.")
fig.tight_layout()
fig.savefig(a.output / "rectification.png")
plt.close(fig)

candidates = json.loads((ROOT / "tests/fixtures/blender/candidate-faults.json").read_text())
fig, axes = plt.subplots(1, 2, figsize=(11, 4))
for state, label in [(0, "healthy"), (1, "D1-open selected"), (2, "D2-open audit only")]:
    rows = [x for x in candidates if x["fault"] == state and x["kind"] == "held"]
    axes[0].semilogx([r["peak_input"] for r in rows], [r["rms"] for r in rows], "o-", label=label)
    axes[1].semilogx([r["peak_input"] for r in rows], [r["h2_f_db"] for r in rows], "o-", label=label)
axes[0].set_ylabel("jack RMS volts")
axes[1].set_ylabel("2f/f dB, fitted196/392Hz")
for ax in axes:
    ax.set_xlabel("input peak volts")
    ax.grid(alpha=.25)
    ax.legend()
fig.suptitle("Gate0 independent candidates — transient20–40ms window, not universal steady-state THD")
fig.tight_layout()
fig.savefig(a.output / "fault-selection.png")
plt.close(fig)

rows = list(csv.DictReader((ROOT / "docs/data/sf016/alias.csv").open()))
fig, axes = plt.subplots(1, 2, figsize=(11, 4))
for kind, label in [(0, "~1760Hz"), (1, "~3520Hz"), (2, "~8kHz"), (3, "chord")]:
    x, y, z = [], [], []
    for factor in [1, 2, 4, 8]:
        selected = [r for r in rows if int(r["kind"]) == kind and int(r["factor"]) == factor]
        x.append(factor)
        y.append(100 * max(float(r["relative_RMS"]) for r in selected))
        z.append(max([float(r["nonharmonic_error_line_dBr"]) for r in selected if r["nonharmonic_error_line_dBr"]] or [-300]))
    axes[0].plot(x, y, "o-", label=label)
    if kind < 3:
        axes[1].plot(x, z, "o-", label=label)
axes[0].set_yscale("log")
axes[0].set_ylabel("worst full-band relative RMS error %")
axes[1].set_ylabel("worst nonharmonic error line dBr")
axes[1].axhline(-45, color="black", ls=":", label="−45dBr gate")
for ax in axes:
    ax.set_xticks([1, 2, 4, 8])
    ax.set_xlabel("complete processor factor")
    ax.grid(alpha=.25)
    ax.legend()
fig.suptitle("Strict sampling gates FAILED;1x/2x include rejected solver failures; wanted2f masked\n14/48 direct16x/32x references lack required convergence; error lines include numerical/transient error.")
fig.tight_layout()
fig.savefig(a.output / "sampling.png")
plt.close(fig)


def wav(path):
    with wave.open(str(path), "rb") as w:
        assert w.getnchannels() == 1 and w.getsampwidth() == 2
        rate = w.getframerate()
        return rate, np.frombuffer(w.readframes(w.getnframes()), dtype="<i2").astype(float) / 32768


fig, axes = plt.subplots(2, 1, figsize=(12, 6), sharex=True)
for suffix, label in [("external_dry", "external dry"), ("healthy", "healthy"), ("D1_open", "D1 open")]:
    rate, x = wav(a.renders / ("input_sustain_mid_b100_boost0_" + suffix + "_raw.wav"))
    hop = rate // 20
    blocks = x[:len(x) // hop * hop].reshape(-1, hop)
    t = (np.arange(len(blocks)) + .5) * hop / rate
    axes[0].plot(t, np.sqrt(np.mean(blocks * blocks, axis=1)), label=label)
    rate, matched = wav(a.renders / ("input_sustain_mid_b100_boost0_" + suffix + "_rms_matched.wav"))
    b = matched[:len(matched) // hop * hop].reshape(-1, hop)
    axes[1].plot(t, np.sqrt(np.mean(b * b, axis=1)), label=label)
axes[0].set_ylabel("raw RMS, native volts/8")
axes[1].set_ylabel("individual offline RMS match")
axes[1].set_xlabel("seconds, exact20s take03 +2s tail")
for ax in axes:
    ax.legend()
    ax.grid(alpha=.25)
fig.suptitle("Same-take actual native PCM16; genuine fault level change retained in raw/common\nRMS matching has disclosed gains and is not LUFS/perceptual approval.")
fig.tight_layout()
fig.savefig(a.output / "guitar.png")
plt.close(fig)
