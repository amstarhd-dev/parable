#!/usr/bin/env python3
"""Independent ngspice stamps for the ADR-004 reduced circuit; no native DSP input."""
import argparse
import bisect
import csv
import hashlib
import json
import math
import subprocess
from pathlib import Path
from rat_spice_reference import run_spice  # Existing neutral simulator/log wrapper, unchanged.

# Literal independent expression of Koeper/Holters equations 9/17/18 and fit facts.
# Parameter functions freeze outside their measured |Vgs| window, as ADR-004 states.
def device_netlist():
    # ngspice expands these single-level functions reliably; nested user functions
    # failed to expand in 44.2. This is independent literal parameter algebra.
    q = 'min(6,max(2.5,u))'
    an = f'(0.020662094888127674-0.0017181795239085821*({q}))'
    tn = f'(1.208306917691355+0.3139084341943607*({q}))'
    ap = f'(-0.0003577445606469842+0.0008620153809796321*({q})-0.00016848836814836602*({q})^2+0.000010800821774906936*({q})^3)'
    tp = f'(0.25610349392710086+0.27051216771368214*({q}))'
    functions = []
    for name, alpha, threshold, modulation in [('channel_n', an, tn, '1'), ('channel_p', ap, tp, '(1+0.06*d)')]:
        over = f'(u-{threshold})'
        shape = f'(d<={over} ? ({over}-d/2)*d : {over}^2/2)'
        functions.append(f'.func {name}(u,d) {{u<={threshold} ? 0 : {alpha}*{shape}*{modulation}}}')
    return ('\n.subckt inverter gate out rail\n'+'\n'.join(functions)+
            '\nBpair out 0 I={channel_n(v(gate),v(out))-channel_p(v(rail)-v(gate),v(rail)-v(out))}\n.ends\n')


DEVICE = device_netlist()


def reference(executable, output, name, circuit, analysis, vector):
    # Prevent a stale raw file from satisfying the unchanged shared runner's
    # existence check after a failed analysis. Keep errors/netlists in its logs.
    (output / (name+'.txt')).unlink(missing_ok=True)
    return run_spice(executable, output, name, circuit, analysis, vector)


def generate(executable, output, fixtures):
    output.mkdir(parents=True, exist_ok=True)
    fixtures.mkdir(parents=True, exist_ok=True)
    (output / 'spinit').write_text('* Explicit CMOS reference startup\nset num_threads=1\n')
    version = subprocess.check_output([executable, '--version'], text=True).strip()
    if 'ngspice-44.2' not in version:
        raise ValueError('Use pinned ngspice 44.2; package hash/URL in cmos provenance')
    dc = []
    for load in (0, 1e-5):
        circuit = 'Independent CD4049 extended-fit DC reference\nVrail rail 0 9\nVin gate 0 0\nXpair gate out rail inverter\n.nodeset V(out)=9\n'
        if load:
            circuit += f'Rload out 0 {1/load:.17g}\n'
        rows = reference(executable, output, f'dc-{load}', circuit+DEVICE, 'dc Vin 0 9 0.05', 'v(out)')
        if len(rows) != 181 or any(not -.001 <= row[1] <= 9.001 for row in rows):
            raise ValueError('DC reference escaped the published physical drain-voltage domain')
        dc.extend([load, *row] for row in rows)
    with (fixtures / 'inverter-dc.csv').open('w', newline='') as stream:
        writer = csv.writer(stream, lineterminator='\n')
        writer.writerow(['load_siemens', 'gate_volts', 'output_volts'])
        writer.writerows(dc)
    cases = [('healthy_low', 0, 33e-9, 1e6), ('healthy_mid', .5, 33e-9, 1e6),
             ('healthy_high', 1, 33e-9, 1e6), ('coupling_small', .5, 330e-12, 1e6),
             ('feedback_low', .5, 33e-9, 100e3)]
    transient = []
    for name, drive, coupling, feedback in cases:
        circuit = f'''Independent two-inverter reduced circuit transient reference
Vrail rail 0 9
Vin source 0 SIN(0 0.1 997)
Rin source intermediate 100k
Cin intermediate g1 68n
Rf1 g1 o1 {100e3+drive*1e6:.17g}
Cf1 g1 o1 51p
Xfirst g1 o1 rail inverter
Cinter o1 g2 {coupling:.17g}
Rf2 g2 o2 {feedback:.17g}
Cf2 g2 o2 100p
Xsecond g2 o2 rail inverter
Cout o2 top 10u
Rtop top jack 5k
Rbottom jack 0 5k
Rload jack 0 1Meg
.nodeset V(g1)=3.48 V(o1)=3.48 V(g2)=3.48 V(o2)=3.48
'''
        rows = reference(executable, output, name, circuit+DEVICE,
                         'tran 0.5u 0.204 0.2 0.5u', 'v(g1) v(o1) v(g2) v(o2) v(jack)')
        if not rows or any(not all(math.isfinite(value) for value in row)
                           or not (0 <= row[2] <= 9 and 0 <= row[4] <= 9) for row in rows):
            raise ValueError('Transient reference escaped the physical output-voltage domain')
        times = [row[0] for row in rows]
        for sample in range(19201, 19585, 2):
            time = sample/96000
            hi = bisect.bisect_left(times, time)
            lo, hi = max(0, hi-1), min(hi, len(rows)-1)
            if not times[lo] <= time <= times[hi]:
                raise ValueError('Missing transient span')
            mix = (time-times[lo])/(times[hi]-times[lo]) if hi != lo else 0
            values = [rows[lo][i]+mix*(rows[hi][i]-rows[lo][i]) for i in range(1, 6)]
            transient.append([name, drive, sample, time, *values])
    with (fixtures / 'circuit-transient.csv').open('w', newline='') as stream:
        writer = csv.writer(stream, lineterminator='\n')
        writer.writerow(['case', 'drive', 'sample_96khz', 'seconds', 'gate1_volts', 'output1_volts',
                         'gate2_volts', 'output2_volts', 'jack_volts'])
        writer.writerows(transient)
    manifest = dict(simulator=version, scope='Independent solver of ADR-004 reduced circuit, not new physical CD4049 measurements',
                    startup_sha256=hashlib.sha256((output/'spinit').read_bytes()).hexdigest(),
                    netlists_sha256={p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(output.glob('*.cir'))},
                    fixtures_sha256={p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(fixtures.glob('*.csv'))})
    (fixtures/'spice-manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('Generated independent CMOS DC/transient fixtures:', fixtures)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ngspice', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--fixtures', type=Path, required=True)
    args = parser.parse_args()
    generate(args.ngspice, args.output, args.fixtures)
