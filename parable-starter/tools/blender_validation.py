#!/usr/bin/env python3
"""Observe native/reference field errors and independent board DC KCL.

Offline only. Uses literal board resistors/terminal connections, not native matrix
stamps. DC snapshots have zero capacitor current. Other DC points are checked by
the native80-case voltage/current test; this KCL audit covers48 traced snapshots.
"""
import argparse
import csv
import json
from pathlib import Path
import struct
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--native", type=Path, required=True)
p.add_argument("--output", type=Path, required=True)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
fixture = ROOT / "tests/fixtures/blender"
manifest = json.loads((fixture / "spice-manifest.json").read_text())
cases = list(csv.DictReader((fixture / "cases.csv").open()))


def load(path):
    b = path.read_bytes()
    assert b[:8] == b"SF016F32"
    rows, columns = struct.unpack_from("<II", b, 8)
    assert columns == 50 and len(b) == rows * 208 + 16
    return np.frombuffer(b, dtype=np.dtype([("case", "<u4"), ("frame", "<u4"), ("v", "<f4", (50,))]), offset=16)


expected, actual = load(fixture / "transient.f32"), load(a.native)
assert np.array_equal(expected["case"], actual["case"]) and np.array_equal(expected["frame"], actual["frame"])


def resistance(text):
    s = text.lower()
    for suffix, factor in [("meg", 1e6), ("k", 1e3)]:
        if s.endswith(suffix):
            return float(s[:-len(suffix)]) * factor
    return float(s)


def kcl(case, row):
    tag = case["kind"]
    name = f'tran-{case["fault"]}-' + ("bypass" if tag == "bypass" else "HF" + str(int(float(case["hz"]))) if tag == "high" else tag)
    text = (fixture / "netlists" / (name + ".cir")).read_text().split(".control")[0]
    volt = dict(zip(manifest["columns"][:31], map(float, row[:31])))
    volt.update({"0": 0., "rail": float(case["supply"]), "source": 0.})
    # These series-only intermediate nodes carry no current at stopped DC.
    volt.update(ci=volt["input"], lb=volt["b5"], f5=volt["b5"], toneunused=volt["tin"])
    parent = {n: n for n in volt}

    def root(n):
        while parent[n] != n:
            n = parent[n]
        return n

    parts = [line.split() for line in text.splitlines()[1:] if line and line[0] in "RVDQ"]
    for part in parts:
        if part[0].startswith("V") and float(part[3]) == 0:
            parent[root(part[1])] = root(part[2])
    sums = {root(n): 0. for n in volt}

    def add(node, value):
        sums[root(node)] += value

    for part in parts:
        name = part[0]
        if name.startswith("R"):
            current = (volt[part[1]] - volt[part[2]]) / resistance(part[3])
            add(part[1], current)
            add(part[2], -current)
        elif name.startswith("Q"):
            q = int(name[1:]) - 1
            ib, ic, ie = map(float, row[31 + 3*q:34 + 3*q])
            for n, current in zip(part[1:4], [ic, ib, ie]):
                add(n, current)
        elif name.startswith("D"):
            current = float(row[46 + int(name[1:]) - 1])
            add(part[1], current)
            add(part[2], -current)
    fixed = {root(n) for n in ["0", "source", "rail"]}
    return max(abs(v) for n, v in sums.items() if n not in fixed)


fields, summaries, balance = [], [], []
for i, case in enumerate(cases):
    mask = expected["case"] == i
    x, y = expected["v"][mask].astype(float), actual["v"][mask].astype(float)
    assert np.isfinite(x).all() and np.isfinite(y).all()
    error = np.sqrt(np.mean((x-y)**2, axis=0))
    ac = np.std(x, axis=0)
    limit = .03*ac + np.r_[np.full(30, .0005), 50e-6, np.full(19, .1e-6)]
    for n, column in enumerate(manifest["columns"]):
        fields.append(dict(case=i, kind=case["kind"], field=column, RMS_error_V_or_A=float(error[n]), allowed_V_or_A=float(limit[n]), peak_error_V_or_A=float(max(abs(x[:, n]-y[:, n]))), gate=bool(error[n]<=limit[n])))
    dc = y[actual["frame"][mask] == 0]
    assert len(dc) == 1
    residual = kcl(case, dc[0])
    balance.append(dict(case=i, kind=case["kind"], peak_DC_KCL_A=residual, gate=residual<=2e-9))
    summaries.append(dict(case=i, kind=case["kind"], failed_fields=int(sum(error>limit)), jack_RMS_error_V=float(error[30]), jack_gate=bool(error[30]<=limit[30])))
for name, rows in [("physical-fields.csv", fields), ("dc-kcl.csv", balance)]:
    with (a.output / name).open("w", newline="") as f:
        w = csv.DictWriter(f, rows[0].keys(), lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
result = dict(scientific_verdict="FAILED", transient_conditions=len(cases), passing_conditions=sum(r["failed_fields"]==0 for r in summaries), failed_fields=sum(r["failed_fields"] for r in summaries), DC_KCL_snapshots=len(balance), DC_KCL_peak_A=max(r["peak_DC_KCL_A"] for r in balance), DC_KCL_gate=all(r["gate"] for r in balance), observation_quantization="Float32 native/reference trace; primary C++ gates use double accumulation before trace rounding.", conditions=summaries)
(a.output / "physical-summary.json").write_text(json.dumps(result, indent=2)+"\n")
print(json.dumps({k:v for k,v in result.items() if k!="conditions"}, indent=2))
