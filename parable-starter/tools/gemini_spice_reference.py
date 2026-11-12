#!/usr/bin/env python3
"""SF-005 independent literal circuit stamps; reuse the existing ngspice device reference."""
import argparse
import bisect
import csv
import hashlib
import json
import subprocess
from pathlib import Path
from cmos_spice_reference import DEVICE, reference


def circuit(rail, feedback, leak, amplitude):
    # Rb is the only added fault branch. No native coefficients/expected outputs.
    net = f'''Independent Gemini bias-network reference
Vrail rail 0 {rail}
Vin source 0 SIN(0 {amplitude} 997)
Rin source intermediate 100k
Cin intermediate g1 68n
Rf1 g1 o1 850k
Cf1 g1 o1 51p
Xfirst g1 o1 rail inverter
Cinter o1 g2 33n
Rf2 g2 o2 {feedback}
Cf2 g2 o2 100p
Xsecond g2 o2 rail inverter
Cout o2 top 10u
Rtop top jack 5k
Rbottom jack 0 5k
Rload jack 0 1Meg
.nodeset V(g1)={rail*.387} V(o1)={rail*.387} V(g2)={rail*(.18 if leak else .387)} V(o2)={rail*(.999 if leak else .387)}
'''
    if leak:
        net += 'Rbias_leak g2 0 220k\n'
    return net+DEVICE


def generate(executable, output, fixtures):
    output.mkdir(parents=True, exist_ok=True)
    fixtures.mkdir(parents=True, exist_ok=True)
    (output/'spinit').write_text('* Explicit Gemini startup\nset num_threads=1\n')
    version = subprocess.check_output([executable, '--version'], text=True)
    if 'ngspice-44.2' not in version:
        raise ValueError('Use existing pinned ngspice 44.2')
    dc = []
    for rail in (6, 9):
        for name, feedback, leak in [('healthy', '1Meg', False), ('feedback_low', '100k', False), ('bias_leak', '1Meg', True)]:
            rows = reference(executable, output, f'dc-{name}-{rail}', circuit(rail, feedback, leak, 0), 'op', 'v(g1) v(o1) v(g2) v(o2) v(jack)')
            values = rows[-1][1:]
            if len(values) != 5 or not all(0 <= x <= rail for x in values[:4]):
                raise ValueError('DC root outside physical domain')
            dc.append([name, rail, *values])
    with (fixtures/'bias-dc.csv').open('w') as f:
        writer = csv.writer(f, lineterminator='\n')
        writer.writerow(['case', 'rail_volts', 'gate1', 'output1', 'gate2', 'output2', 'jack'])
        writer.writerows(dc)
    transient = []
    production = []
    playback = []
    for name, fault in [('healthy', False), ('bias_leak', True)]:
        for amplitude in (.05, .15):
            rows = reference(executable, output, f'transient-{name}-{amplitude}', circuit(9, '1Meg', fault, amplitude),
                             'tran 0.5u 0.205 0.2 0.5u', 'v(g1) v(o1) v(g2) v(o2) v(jack)')
            times = [r[0] for r in rows]
            for sample in range(19201, 19585, 2):
                t = sample/96000
                hi = bisect.bisect_left(times, t)
                lo, hi = max(0, hi-1), min(hi, len(rows)-1)
                if not times[lo] <= t <= times[hi]:
                    raise ValueError('Missing transient span')
                mix = (t-times[lo])/(times[hi]-times[lo])
                values = [rows[lo][i]+mix*(rows[hi][i]-rows[lo][i]) for i in range(1, 6)]
                if not all(0 <= values[i] <= 9 for i in (1, 3)):
                    raise ValueError('Transient escaped rail domain')
                transient.append([name, amplitude, sample, *values])
            # Actual 4x last-phase nodes: input FIR delay=96 internal samples
            # (24 base samples); last phase=3/4 base sample. Analytic alignment,
            # not fitted cross-correlation or output-decimator delay compensation.
            for sample in range(19249, 19633, 2):
                t = (sample-24+.75)/96000
                hi = bisect.bisect_left(times, t)
                lo, hi = hi-1, hi
                mix = (t-times[lo])/(times[hi]-times[lo])
                values = [rows[lo][i]+mix*(rows[hi][i]-rows[lo][i]) for i in range(1, 6)]
                production.append([name, amplitude, sample, *values])
            for rate in (44100, 48000):
                # Verify the actual audition and Seed3 base rates as well, using
                # the same independently simulated time span and analytical delay.
                start = round(.2*rate)+24
                for sample in range(start, start+192, 2):
                    t = (sample-24+.75)/rate
                    hi = bisect.bisect_left(times, t)
                    lo = hi-1
                    if lo < 0 or hi >= len(rows):
                        raise ValueError('Missing playback reference span')
                    mix = (t-times[lo])/(times[hi]-times[lo])
                    values = [rows[lo][i]+mix*(rows[hi][i]-rows[lo][i]) for i in range(1, 6)]
                    playback.append([name, amplitude, rate, sample, *values])
    with (fixtures/'bias-transient.csv').open('w') as f:
        writer = csv.writer(f, lineterminator='\n')
        writer.writerow(['case', 'input_peak_volts', 'sample_96khz', 'gate1', 'output1', 'gate2', 'output2', 'jack'])
        writer.writerows(transient)
    with (fixtures/'bias-production-4x.csv').open('w') as f:
        writer = csv.writer(f, lineterminator='\n')
        writer.writerow(['case', 'input_peak_volts', 'sample_96khz', 'gate1', 'output1', 'gate2', 'output2', 'jack'])
        writer.writerows(production)
    with (fixtures/'bias-playback-4x.csv').open('w') as f:
        writer = csv.writer(f, lineterminator='\n')
        writer.writerow(['case', 'input_peak_volts', 'sample_rate_hz', 'sample', 'gate1', 'output1', 'gate2', 'output2', 'jack'])
        writer.writerows(playback)
    manifest = dict(simulator=version.strip(), scope='Independent solver of extended SF-002 reduced model; not physical hardware validation',
                    component_change='gate2-to-VSS normally open -> 220000 ohm leakage', max_step_seconds=5e-7,
                    production_alignment='native phase 3 at (sample-24+0.75)/96000 seconds; FIR group delay, no fitted shift',
                    netlists_sha256={p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(output.glob('*.cir'))},
                    fixtures_sha256={p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(fixtures.glob('bias-*.csv'))})
    manifest['playback_alignment'] = 'native phase 3 at (sample-24+0.75)/rate; exact 44100/48000 Hz'
    (fixtures/'spice-manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ngspice', required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--fixtures', type=Path, required=True)
    args = p.parse_args()
    generate(args.ngspice, args.output, args.fixtures)
