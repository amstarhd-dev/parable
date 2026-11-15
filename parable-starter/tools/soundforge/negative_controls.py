#!/usr/bin/env python3
"""Exercise three original gates against isolated deliberate corruption only."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--project', type=Path, required=True)
p.add_argument('--build', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--compiler', default='c++')
a = p.parse_args()
project, build, out = a.project.resolve(), a.build.resolve(), a.output.resolve()
out.mkdir(parents=True, exist_ok=False)
# Never mutate a checked-in reference, audio input, or golden.
with tempfile.TemporaryDirectory(prefix='sf-negative-') as d:
    root = Path(d)
    fixtures = root / 'reference'
    fixtures.mkdir()
    reference = project / 'tests/fixtures/ts808/device.csv'
    with reference.open() as f:
        rows = list(csv.reader(f))
    rows[1][2] = str(float(rows[1][2]) + 1.0)  # +1 amp expected base current
    with (fixtures / 'device.csv').open('w') as f:
        csv.writer(f).writerows(rows)
    flags = ['-Wl,--wrap=' + x for x in ['malloc', 'calloc', 'realloc', 'free',
             'pthread_mutex_lock', 'pthread_mutex_trylock', 'pthread_cond_wait',
             'read', 'write', 'fopen', 'fwrite']]
    native = root / 'ts808-reference-mutant'
    command = [a.compiler, '-std=c++17', '-g', '-O0',
               '-DTS808_FIXTURES="' + str(fixtures) + '"',
               *['-I' + str(project / folder) for folder in ['include', 'src', 'tools', 'tests']],
               str(project / 'tests/test_ts808.cpp'), str(project / 'tests/activation_guard.cpp'),
               str(build / 'libparable_ts808.a'), '-pthread', *flags, '-o', str(native)]
    subprocess.run(command, check=True, capture_output=True, text=True)
    # Run the ORIGINAL CLI assertions with just its generated source corrupted.
    cli = (project / 'tests/test_ts808_cli.py').read_text()
    marker = '    digest=hashlib.sha256(source.read_bytes()).hexdigest()'
    assert cli.count(marker) == 1
    cli = cli.replace(marker, "    source.write_bytes(b'invalid WAV')\n" + marker)
    (root / 'invalid-audio-cli.py').write_text(cli)
    golden = json.loads((project / 'tests/fixtures/soundforge/labrat-main.json').read_text())
    name = next(iter(golden['outputs_sha256']))
    golden['outputs_sha256'][name] = '0' * 64
    (root / 'golden.json').write_text(json.dumps(golden))
    checker = (project / 'tools/soundforge/check_labrat.py').read_text()
    checker = checker.replace('PROJECT = Path(__file__).resolve().parents[2]',
                              'PROJECT = Path(' + repr(str(project)) + ')')
    checker = checker.replace('BASELINE = PROJECT / "tests/fixtures/soundforge/labrat-main.json"',
                              'BASELINE = Path(' + repr(str(root / 'golden.json')) + ')')
    (root / 'golden-mutant.py').write_text(checker)
    tests = {
        'ts808_circuit_and_safety': [str(native)],
        'ts808_listening_cli': [sys.executable, str(root / 'invalid-audio-cli.py'), str(build / 'parable_ts808_lab')],
        'labrat_output_preservation': [sys.executable, str(root / 'golden-mutant.py'), '--executable', str(build / 'parable_rat_lab')],
    }
    launch = []
    for name, args in tests.items():
        launch.append('add_test(' + name + ' ' + ' '.join('"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"' for s in args) + ')')
    (root / 'CTestTestfile.cmake').write_text('\n'.join(launch) + '\n')
    result = subprocess.run(['ctest', '--test-dir', str(root), '-j1', '--output-on-failure', '--no-tests=error'], capture_output=True, text=True)
    log = result.stdout + result.stderr
    (out / 'ctest.log').write_text(log)
    assert result.returncode == 8, log
    assert '0% tests passed, 3 tests failed out of 3' in log, log
    assert 'e<1e-10+2e-5*std::abs(p.second)' in log, log
    assert 'WAV must be 44 bytes to 6 MiB' in log, log
    assert 'LabRAT output differs from main:' in log, log
    record = {'isolated_ctest_exit': result.returncode, 'original_gates_failed': list(tests),
              'independent_reference_change': '+1 A first TS808 base-current expectation',
              'audio_change': 'original CLI generated source replaced by invalid WAV bytes',
              'golden_change': 'one LabRAT expected output hash zeroed',
              'original_reference_sha256': hashlib.sha256(reference.read_bytes()).hexdigest(),
              'compile_command': command, 'protected_files_mutated': False,
              'scope': 'three targeted original gates, not a second complete regression suite'}
    (out / 'report.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))
