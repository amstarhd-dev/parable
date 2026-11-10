#!/usr/bin/env python3
"""Exact Linux/GCC before-after regression, not independent circuit validation."""
import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[2]
BASELINE = PROJECT / "tests/fixtures/soundforge/labrat-main.json"


def check(directory):
    baseline = json.loads(BASELINE.read_text())
    # Only the renderer's original files belong here; CI provenance files may coexist.
    expected = baseline["outputs_sha256"]
    names = {path.name for path in directory.iterdir() if path.suffix in (".wav", ".csv")}
    required = {name for name in expected if Path(name).suffix in (".wav", ".csv")}
    if names != required:
        raise ValueError("Changed LabRAT WAV/CSV output set")
    for name, digest in expected.items():
        if hashlib.sha256((directory / name).read_bytes()).hexdigest() != digest:
            raise ValueError(f"LabRAT output differs from main: {name}")
    for name, digest in baseline["protected_sha256"].items():
        if hashlib.sha256((PROJECT / name).read_bytes()).hexdigest() != digest:
            raise ValueError(f"Protected LabRAT source/reference differs from main: {name}")
    return dict(baseline_commit=baseline["baseline_commit"],
                output_files_byte_identical=len(expected), wav_files_byte_identical=11,
                protected_files_identical=len(baseline["protected_sha256"]),
                scope="Linux/GCC exact regression only; not circuit fidelity or musical acceptance")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--directory", type=Path)
    mode.add_argument("--executable", type=Path)
    args = parser.parse_args()
    try:
        if args.directory:
            report = check(args.directory)
        else:
            with tempfile.TemporaryDirectory(prefix="soundforge-labrat-") as directory:
                subprocess.run([str(args.executable.resolve()), "--output", directory],
                               check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=90)
                report = check(Path(directory))
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        parser.exit(1, f"LabRAT compatibility failure: {error}\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
