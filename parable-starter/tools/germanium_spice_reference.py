#!/usr/bin/env python3
"""Pinned independent PNP solve of ADR005; never reads native DSP output."""
import argparse
import bisect
import csv
import hashlib
import json
import math
import shutil
import subprocess
from pathlib import Path
from rat_spice_reference import run_spice

# Paper equations1,4-7, independently stamped as physical-sign PNP current sources.
# Constants agree with Table1/MIT Whitehead transcription. No copied MATLAB/Julia.
DEVICE = '''
.subckt measured c b e params: beta=229.6 leak=7.546u
Bbe e b I={20.66u/beta*(exp(v(e,b)/(1.133*0.0255))-1)+2.190u*(exp(v(e,b)/(1.796*0.0255))-1)}
Bbc c b I={20.66u/14.66*(exp(v(c,b)/(1.140*0.0255))-1)+leak*(exp(v(c,b)/(1.364*0.0255))-1)}
Btransport e c I={2*(1-v(e,b)/88.28-v(c,b)/19.68)/(1+sqrt(1+4*(20.66u*(exp(v(e,b)/(1.133*0.0255))-1)/0.463+20.66u*(exp(v(c,b)/(1.140*0.0255))-1)/0.2415)))*(20.66u*(exp(v(e,b)/(1.133*0.0255))-1)-20.66u*(exp(v(c,b)/(1.140*0.0255))-1))}
.ends
'''
CASES = [('healthy_low', 0., 1000., .05, 7.546e-6, 100e3),
         ('healthy_mid', .5, 1000., .05, 7.546e-6, 100e3),
         ('healthy_high', 1., 1000., .05, 7.546e-6, 100e3),
         ('source_high', .8, 50e3, .05, 7.546e-6, 100e3),
         ('weak_input', .8, 1000., .005, 7.546e-6, 100e3),
         ('q1_leaky', .8, 1000., .05, 75.46e-6, 100e3),
         ('feedback_wrong', .8, 1000., .05, 7.546e-6, 10e3)]


def circuit(fuzz, source, amplitude, leak, feedback):
    # Exact endpoint pots use wires (zero-volt sources), not tiny fake resistors.
    top = f'Rft e2 wiper {(1-fuzz)*1000:.17g}' if fuzz < 1 else 'Vft e2 wiper 0'
    bottom = f'Rfb wiper 0 {fuzz*1000:.17g}' if fuzz > 0 else 'Vfb wiper 0 0'
    return f'''Positive-ground PNP reduced germanium circuit
Vbattery supply 0 -9
Vinput source 0 SIN(0 {amplitude:.17g} 997)
Rsource source intermediate {source:.17g}
Cin intermediate b1 2.2u
Xq1 c1 b1 0 measured leak={leak:.17g}
Xq2 c2 c1 e2 measured
Rcollector1 c1 supply 33k
Rfeedback b1 e2 {feedback:.17g}
{top}
{bottom}
Cbypass wiper 0 20u
Cbc1 b1 c1 100p
Cbc2 c1 c2 100p
Rcollector2 c2 tap 8.2k
Rsupplytap tap supply 470
Cout tap volume 10n
RvolumeTop volume jack 250k
RvolumeBottom jack 0 250k
Rload jack 0 1Meg
.nodeset v(b1)=-.073 v(c1)=-.21 v(e2)=-.16 v(c2)=-7.55
''' + DEVICE


def reference(executable, output, name, network, analysis, vectors):
    (output / (name+'.txt')).unlink(missing_ok=True)
    rows = run_spice(executable, output, name, network, analysis, vectors)
    if not rows or any(not all(math.isfinite(v) for v in row) for row in rows):
        raise ValueError('Empty/nonfinite independent reference: '+name)
    return rows


def write_csv(path, header, rows):
    with path.open('w', newline='') as stream:
        writer = csv.writer(stream, lineterminator='\n')
        writer.writerow(header)
        writer.writerows(rows)


def generate(executable, output, fixtures):
    output.mkdir(parents=True, exist_ok=True)
    fixtures.mkdir(parents=True, exist_ok=True)
    (output/'spinit').write_text('* SF003 independent PNP reference\nset num_threads=1\n')
    version = subprocess.check_output([executable, '--version'], text=True).strip()
    if 'ngspice-44.2' not in version:
        raise ValueError('Use the existing SHA-pinned ngspice44.2 package')
    devices = []
    for name, beta, leak in [('nominal',229.6,7.546e-6),('low_gain',100.,7.546e-6),('leaky',229.6,75.46e-6)]:
        network = f'''Independent measured PNP current reference
Vcollector c 0 -2
Vbase b 0 0
Xtest c b 0 measured beta={beta:.17g} leak={leak:.17g}
''' + DEVICE
        rows = reference(executable, output, 'device-'+name, network,
                         'dc Vbase 0.1 -0.3 -0.01', 'v(b) i(Vbase) i(Vcollector)')
        if len(rows) != 41:
            raise ValueError('Incomplete device sweep')
        devices.extend([name,beta,leak,-r[1],-2-r[1],r[2],r[3]] for r in rows)
    write_csv(fixtures/'device-current.csv', ['case','beta','bc_leakage_amps','ve_volts','vc_volts','base_amps','collector_amps'], devices)
    bias, transient = [], []
    vectors = 'v(b1) v(c1) v(e2) v(c2) v(tap) v(jack) i(Vinput)'
    for name,fuzz,source,amplitude,leak,feedback in CASES:
        network = circuit(fuzz,source,amplitude,leak,feedback)
        dc = reference(executable,output,'bias-'+name,network,'op',vectors)[0]
        if any(not -9.001 <= dc[i] <= .001 for i in range(1,6)):
            raise ValueError('Unphysical DC branch '+name)
        bias.append([name,fuzz,source,leak,feedback,*dc[1:7]])
        rows = reference(executable,output,'transient-'+name,network,'tran 0.25u 0.012 0 0.25u',vectors)
        times = [r[0] for r in rows]
        for sample in range(1,1153,6):
            time = sample/96000
            hi = bisect.bisect_left(times,time)
            lo,hi = max(0,hi-1),min(hi,len(rows)-1)
            if not times[lo] <= time <= times[hi]:
                raise ValueError('Missing transient span')
            mix = (time-times[lo])/(times[hi]-times[lo]) if hi!=lo else 0
            values = [rows[lo][i]+mix*(rows[hi][i]-rows[lo][i]) for i in range(1,8)]
            values[-1] = -values[-1]  # Current entering pedal, opposite source current.
            transient.append([name,fuzz,source,amplitude,leak,feedback,sample,*values])
    write_csv(fixtures/'bias.csv',['case','fuzz','source_ohms','q1_bc_leakage_amps','feedback_ohms','base1_volts','collector1_volts','emitter2_volts','collector2_volts','tap_volts','jack_volts'],bias)
    write_csv(fixtures/'transient.csv',['case','fuzz','source_ohms','input_volts','q1_bc_leakage_amps','feedback_ohms','sample_96khz','base1_volts','collector1_volts','emitter2_volts','collector2_volts','tap_volts','jack_volts','input_amps'],transient)
    netlists=fixtures/'netlists';netlists.mkdir(exist_ok=True)
    for path in output.glob('*.cir'):
        shutil.copyfile(path,netlists/path.name)
    manifest=dict(simulator=version,scope='Independent solver of explicit ADR005 equations/network; not physical pedal measurements',
                  thermal_volts=.0255,wrapper_temperature_c=26.85,maximum_transient_step_seconds=.25e-6,
                  startup_sha256=hashlib.sha256((output/'spinit').read_bytes()).hexdigest(),
                  netlists_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(netlists.glob('*.cir'))},
                  fixtures_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(fixtures.glob('*.csv'))})
    (fixtures/'spice-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Generated independent PNP device/DC/transient fixtures:',fixtures)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ngspice',required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--fixtures',type=Path,required=True)
    args=parser.parse_args()
    generate(args.ngspice,args.output,args.fixtures)
