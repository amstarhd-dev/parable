#!/usr/bin/env python3
"""Independent double-precision dense-matrix difference-equation reference.

No native source parsing/import. This validates the declared reduced network,
not architectural acoustics. Python3 standard library only. Regenerate fixtures.
"""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path


def generate(output):
    output.mkdir(parents=True, exist_ok=True)
    rows = []
    for fault in (0, 1):
        for rate in (44100, 48000, 96000):
            times = [.0113, .0137, .0179, .0197, .0293, .0371, .0437, .0531]
            lengths = [int(t*rate+.5) for t in times]
            gains = [.001**(length/rate/(.65 if i < 4 else 1.9)) for i, length in enumerate(lengths)]
            poles = [math.exp(-2*math.pi*f/rate) for f in (5500, 3200)]
            # Construct separate H, aperture C and loss D; multiply dense matrices.
            h = [[(float(i == j)-.5) if i//4 == j//4 else 0. for j in range(8)] for i in range(8)]
            c = [[float(i == j) for j in range(8)] for i in range(8)]
            c[0][0] = c[4][4] = .15
            c[0][4] = -.85 if fault else .85
            c[4][0] = .85
            m = [[gains[i]*sum(c[i][k]*h[k][j] for k in range(8)) for j in range(8)] for i in range(8)]
            buffers = [[0.]*n for n in lengths]
            positions, filtered = [0]*8, [0.]*8
            for frame in range(rate//2):
                v = [buffers[i][positions[i]] for i in range(8)]
                filtered = [(1-poles[i//4])*v[i]+poles[i//4]*filtered[i] for i in range(8)]
                incoming = [sum(m[i][j]*filtered[j] for j in range(8)) for i in range(8)]
                for i in range(8):
                    buffers[i][positions[i]] = incoming[i]+(.25 if frame == 0 and i < 4 else 0.)
                    positions[i] = (positions[i]+1) % lengths[i]
                a = .5*sum(filtered[i]*s for i, s in enumerate((1, -1, 1, 1)))
                b = .5*sum(filtered[i+4]*s for i, s in enumerate((1, -1, 1, 1)))
                # Sparse fixture covers all timeline, plus exact first-path onsets.
                if frame % 37 == 0 or frame in lengths or frame in (lengths[0]+lengths[4],):
                    rows.append((fault, rate, frame, .5*(a+b), a, b))
    path = output/'impulse.csv'
    with path.open('w', newline='') as f:
        writer = csv.writer(f); writer.writerow(('fault', 'rate', 'frame', 'mono', 'A', 'B')); writer.writerows(rows)
    manifest = dict(reference='Independent Python double dense D*C*H recurrence; no native code imported',
                    rows=len(rows), sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                    generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                    limits='Same declared reduced model, not an independent physical room measurement')
    (output/'reference.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    generate(parser.parse_args().output)
