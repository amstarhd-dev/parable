#!/usr/bin/env python3
"""Package already-built, verified runner evidence. No device access."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import subprocess
import zipfile

import seed3_runner

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
PARENT = '3d091127f154847c6938ff909ceecc1ca942ac85'


def package(build, destination):
    manifest_file = build / 'artifacts.json'
    manifest_sha = seed3_runner.sha(manifest_file.read_bytes())
    manifest = seed3_runner.load_kit(manifest_file, manifest_sha)
    evidence = json.loads((build / 'handoff-audit.json').read_text())
    if {a['artifact_id'] for a in evidence['artifacts']} != {a['artifact_id'] for a in manifest['artifacts']}:
        raise ValueError('Run the handoff ELF audit on this exact build before packaging')
    files = {}
    for path in sorted(build.rglob('*')):
        if path.is_file() and path.suffix not in ('.hex', '.o', '.lst', '.d'):
            files[str(path.relative_to(build))] = path.read_bytes()
    for name in ('seed3_runner.py', 'hardware.py', 'audit_handoff.py'):
        files[name] = (HERE / name).read_bytes()
    for path in sorted(HERE.iterdir()):
        if path.is_file():
            files['harness-source/' + path.name] = path.read_bytes()
    files['README.md'] = (ROOT / 'docs/resources/RUNNER.md').read_bytes()
    files['GATE0.md'] = (ROOT / 'docs/resources/RUNNER_GATE0.md').read_bytes()
    files['dependencies.json'] = (ROOT / 'firmware/dependencies.json').read_bytes()
    # Preserve the exact SDK/toolchain/native provenance bundle of PR #83.
    parent = subprocess.check_output(['git', '-C', str(ROOT.parent), 'show',
                                     PARENT + ':parable-starter/docs/resources/resource-firmware-kit.zip'])
    with zipfile.ZipFile(io.BytesIO(parent)) as archive:
        for name in archive.namelist():
            if name.startswith('licenses/') and not name.endswith('/'):
                files[name] = archive.read(name)
    files['SHA256SUMS'] = ''.join(seed3_runner.sha(data) + '  ' + name + '\n'
                                for name, data in sorted(files.items())).encode()
    destination.mkdir(parents=True, exist_ok=True)
    path = destination / ('seed3-runner-v1-' + manifest_sha[:12] + '.zip')
    with zipfile.ZipFile(str(path), 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, (2026, 11, 20, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)
    with zipfile.ZipFile(str(path)) as archive:
        if archive.testzip() is not None:
            raise ValueError('ZIP CRC failure')
        for name, data in files.items():
            if archive.read(name) != data:
                raise ValueError('ZIP payload changed: ' + name)
    receipt = dict(schema='parable.runner-kit.v1', file=path.name,
                   bytes=path.stat().st_size, sha256=seed3_runner.sha(path.read_bytes()),
                   manifest_sha256=manifest_sha, runner_sha256=seed3_runner.sha(files['seed3_runner.py']),
                   images=len(manifest['artifacts']), physical_acceptance_tested=False)
    (destination / 'kit.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    package(args.build.resolve(), args.output.resolve())
