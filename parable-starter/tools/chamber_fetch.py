#!/usr/bin/env python3
"""Fetch exact, hash-verified upstream sources for SF021 host RESEARCH only.

No project import-policy extension or embedded admission is implied. These are
complete upstream candidates, not a new Parable reverb. Standard library only.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def fetch(cache):
    manifest = json.loads((ROOT/'third_party/chamber/sources.json').read_text())
    entries = [f for repo in manifest['repositories'] for f in repo['files']]

    def one(entry):
        relative = Path(entry['path'])
        if relative.is_absolute() or '..' in relative.parts:
            raise ValueError('Unsafe source path')
        path = cache/relative
        data = path.read_bytes() if path.exists() else urllib.request.urlopen(entry['url'], timeout=30).read()
        if len(data) != entry['bytes'] or hashlib.sha256(data).hexdigest() != entry['sha256']:
            raise ValueError('Pinned source mismatch: '+entry['path'])
        path.parent.mkdir(parents=True, exist_ok=True)
        if not path.exists():
            path.write_bytes(data)
        return entry['path']

    with ThreadPoolExecutor(max_workers=6) as pool:
        verified = list(pool.map(one, entries))
    include = cache/'include'
    include.mkdir(exist_ok=True)
    link = include/'signalsmith-dsp'
    if not link.exists():
        link.symlink_to('../dsp', target_is_directory=True)
    print(f'Verified {len(verified)} pinned files; research only, no production admission')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, required=True)
    fetch(parser.parse_args().cache.resolve())
