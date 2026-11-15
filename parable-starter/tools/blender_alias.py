#!/usr/bin/env python3
"""Offline full-processor accuracy/false-line audit; retain strict gate misses.

NumPy2.3.5 research dependency only. A wanted2f/other in-band harmonic is not
an alias. The false-line metric uses residual outside wanted harmonic bins and
also includes nonharmonic numerical/transient error; it is not pure alias power.
"""
import argparse
import csv
import json
from pathlib import Path
import numpy as np

SIZE = 2048


def bandlimit(y, factor):
    assert len(y) == SIZE * factor and np.isfinite(y).all()
    spec = np.fft.rfft(y) / len(y)
    kept = spec[: SIZE // 2 + 1] * SIZE
    kept[-1] = 2 * kept[-1].real
    return np.fft.irfft(kept, n=SIZE), kept


def fir_response(factor):
    t = np.arange(48 * factor + 1)
    n = t - 24 * factor
    fc = .4 / factor
    h = (2 * fc * np.sinc(2 * fc * n) * (.42 - .5 * np.cos(2 * np.pi * t / (48 * factor)) + .08 * np.cos(4 * np.pi * t / (48 * factor)))).astype(np.float32)
    h = (h / sum(map(float, h))).astype(np.float32)
    response = np.fft.rfft(h, n=SIZE * factor)[: SIZE // 2 + 1]
    return response * np.exp(2j * np.pi * np.arange(SIZE // 2 + 1) * 24 / SIZE)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--study", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--factor", type=int, choices=[1, 2, 4, 8], default=8)
    p.add_argument("--require-gates", action="store_true")
    a = p.parse_args()
    rows = list(csv.DictReader((a.study / "runs.csv").open()))
    assert len(rows) == 288
    groups = {}
    for row in rows:
        key = tuple(row[k] for k in ["rate_hz", "fault", "input_peak_bound_V", "kind"])
        groups.setdefault(key, {})[int(row["factor"])] = row
    results = []
    convergence = []

    def wave(row):
        y = np.fromfile(a.study / (row["id"] + ".f32"), dtype="<f4").astype(float)
        assert len(y) == int(row["frames"]) and np.isfinite(y).all()
        return y

    for key, variants in groups.items():
        assert set(variants) == {1, 2, 4, 8, 16, 32}
        truth, spectrum = bandlimit(wave(variants[32]), 32)
        coarse, _ = bandlimit(wave(variants[16]), 16)
        norm = float(np.sqrt(np.mean(truth * truth)))
        refinement = float(np.sqrt(np.mean((truth - coarse) ** 2)))
        converged = refinement <= .005 * norm + 1e-6
        convergence.append(dict(rate=int(key[0]), fault=int(key[1]), amplitude=float(key[2]), kind=int(key[3]), RMS_16x_32x=refinement, relative_RMS=refinement / norm, gate=converged))
        for factor in [1, 2, 4, 8]:
            row = variants[factor]
            y = wave(row)
            error = y - truth
            relative = float(np.sqrt(np.mean(error * error))) / norm
            false_db = None
            if int(key[3]) < 3:
                wanted = int(row["bin"])
                mask = np.ones(SIZE // 2 + 1, dtype=bool)
                mask[:3] = False
                for harmonic in range(wanted, SIZE // 2 + 1, wanted):
                    mask[max(0, harmonic - 2) : min(len(mask), harmonic + 3)] = False
                window = np.hanning(SIZE)
                residual = 2 * np.abs(np.fft.rfft(error * window)) / window.sum()
                false_db = float(20 * np.log10(max(float(max(residual[mask])), 1e-30) / norm))
            limit = .10 if int(key[3]) == 2 else .05
            clean = all(int(row[k]) == 0 for k in ["failures", "input_clamps", "output_clamps"])
            # Explanatory output-FIR diagnostic never replaces the full-band gate.
            filtered_truth = np.fft.irfft(spectrum * fir_response(factor), n=SIZE) if factor > 1 else truth
            diagnostic = float(np.sqrt(np.mean((y - filtered_truth) ** 2))) / norm
            passed = clean and converged and relative <= limit and (false_db is None or false_db <= -45)
            results.append(dict(rate=int(key[0]), fault=int(key[1]), amplitude=float(key[2]), kind=int(key[3]), bin=int(row["bin"]), actual_Hz=int(row["bin"]) * int(key[0]) / SIZE if int(key[3]) < 3 else 0, factor=factor, relative_RMS=relative, fullband_limit=limit, nonharmonic_error_line_dBr=false_db, FIR_only_diagnostic_relative_RMS=diagnostic, reference_converged=converged, clean=clean, gate=passed, CPU_us_per_base_frame=float(row["cpu_us_per_frame"])))
    a.output.mkdir(parents=True, exist_ok=True)
    with (a.output / "alias.csv").open("w", newline="") as f:
        w = csv.DictWriter(f, results[0].keys(), lineterminator="\n")
        w.writeheader()
        w.writerows(results)
    summary = dict(gates="original Gate0 thresholds unchanged", future_target_cycles="not measured", converged_reference_cases=sum(r["gate"] for r in convergence), reference_cases=len(convergence), selected_factor=a.factor, factors={str(f): dict(passed=sum(r["gate"] for r in results if r["factor"] == f), cases=sum(r["factor"] == f for r in results), worst_fullband_relative_RMS=max(r["relative_RMS"] for r in results if r["factor"] == f), worst_nonharmonic_error_line_dBr=max(r["nonharmonic_error_line_dBr"] for r in results if r["factor"] == f and r["nonharmonic_error_line_dBr"] is not None)) for f in [1, 2, 4, 8]}, convergence=convergence)
    (a.output / "alias-summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps({k: v for k, v in summary.items() if k != "convergence"}, indent=2))
    if a.require_gates and any(not r["gate"] for r in results if r["factor"] == a.factor):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
