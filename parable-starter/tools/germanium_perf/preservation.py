#!/usr/bin/env python3
"""Check regenerated ORIGINAL/libm artifacts against captured unchanged-main bytes."""
import argparse
import hashlib
import json
from pathlib import Path
PROJECT=Path(__file__).resolve().parents[2]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for model in ('rat','cmos','germanium'):
        p.add_argument('--'+model,type=Path,required=True)
    a=p.parse_args()
    for model in ('rat','cmos','germanium'):
        expected=json.loads((PROJECT/'tests/fixtures/germanium-perf'/('before-'+model+'-hashes.json')).read_text())
        for name,digest in expected.items():
            path=getattr(a,model)/name
            if hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
                raise SystemExit(f'{model}/{name}: differs from unchanged-main capture')
        print(f'{model}: {len(expected)} original WAV/report files byte-identical')
if __name__=='__main__':
    main()
