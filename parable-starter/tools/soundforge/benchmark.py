#!/usr/bin/env python3
"""Measure the actual complete CTest suite; no cached passes or gate selection.

POSIX wait4 includes a child's waited-for descendants (e.g. CLI renderers).
The generated CTest launch lines are wrapped only during this invocation, then
restored. Source commands/properties and failure exit codes remain authoritative.
Use a separate build/output directory per invocation; never run CMake or another
CTest against a build being measured. No dependencies beyond CTest/Python.
"""
import argparse
import csv
import hashlib
import io
import json
import os
import resource
import runpy
import shutil
import subprocess
import sys
import time
from pathlib import Path

OBSERVED_CLI = {'test_ts808_cli.py', 'test_klon_cli.py'}
PROFILE_TESTS = {'ts808_circuit_and_safety', 'ts808_listening_cli',
                 'klon_circuit_and_safety', 'klon_listening_cli',
                 'muff_circuit_and_safety', 'wavefolder_circuit_and_safety'}


def artifact_hashes(directory):
    """Only the preexisting nondeterministic host timer column is excluded."""
    result = {}
    for path in sorted(directory.rglob('*')):
        if not path.is_file():
            continue
        data = path.read_bytes()
        if path.name == 'renders.csv':
            rows = list(csv.DictReader(io.StringIO(data.decode())))
            fields = [f for f in rows[0] if f != 'host_processor_us']
            stream = io.StringIO()
            writer = csv.DictWriter(stream, fieldnames=fields, lineterminator='\n')
            writer.writeheader()
            writer.writerows({k: row[k] for k in fields} for row in rows)
            data = stream.getvalue().encode()
        result[str(path.relative_to(directory))] = hashlib.sha256(data).hexdigest()
    return result


def observe_python(output, args):
    """Hash successful actual CLI renders before their existing cleanup runs."""
    original = subprocess.run
    observed = 0

    def run(command, *positional, **keywords):
        nonlocal observed
        result = original(command, *positional, **keywords)
        if result.returncode == 0 and '--output' in command:
            destination = Path(command[command.index('--output') + 1])
            if destination.is_dir():
                output.mkdir(exist_ok=True)
                (output / f'cli-output-{observed}.json').write_text(
                    json.dumps(artifact_hashes(destination), indent=2) + '\n')
                observed += 1
        return result

    subprocess.run = run
    sys.argv = args
    runpy.run_path(args[0], run_name='__main__')


def timed_child(output, name, profile, command):
    if len(command) > 1 and Path(command[1]).name in OBSERVED_CLI:
        command = [command[0], str(Path(__file__).resolve()), '--_observe',
                   str(output / (name + '-artifacts')), *command[1:]]
    if profile and name in PROFILE_TESTS:
        command = ['gprofng', 'collect', 'app', '-a', 'off', '-p', '10',
                   '-o', str(output / (name + '.er')), *command]
    start = time.monotonic()
    process = subprocess.Popen(command)
    _, status, usage = os.wait4(process.pid, 0)
    code = os.waitstatus_to_exitcode(status)
    process.returncode = code
    record = dict(name=name, command=command, start_monotonic_s=start,
                  wall_s=time.monotonic() - start, user_s=usage.ru_utime,
                  system_s=usage.ru_stime, max_rss_KiB=usage.ru_maxrss,
                  exit_code=code)
    (output / (name + '.json')).write_text(json.dumps(record, indent=2) + '\n')
    return code if code >= 0 else 128 - code


def benchmark(build, output, jobs, profile, ctest):
    build, output = build.resolve(), output.resolve()
    # A fresh evidence destination prevents stale successes from being counted.
    output.mkdir(parents=True, exist_ok=False)
    lock = build / '.soundforge-benchmark.lock'
    fd = os.open(lock, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    os.close(fd)
    testfile = build / 'CTestTestfile.cmake'
    original = None
    wrapped = None
    try:
        original = testfile.read_bytes()
        inventory = json.loads(subprocess.check_output(
            [ctest, '--test-dir', str(build), '--show-only=json-v1'], text=True))
        (output / 'inventory.json').write_text(json.dumps(inventory, indent=2) + '\n')
        text = original.decode()
        for test in inventory['tests']:
            name = test['name']
            # CMake's standard generated add_test(NAME ...) representation.
            prefix = 'add_test(' + name + ' '
            if text.count(prefix) != 1:
                raise ValueError('Unsupported generated CTest syntax: ' + name)
            args = [sys.executable, str(Path(__file__).resolve()), '--_child',
                    str(output), name, '1' if profile else '0']
            quoted = ' '.join('"' + x.replace('\\', '\\\\').replace('"', '\\"') + '"'
                              for x in args)
            text = text.replace(prefix, prefix + quoted + ' ', 1)
        wrapped = text.encode()
        testfile.write_bytes(wrapped)
        command = [ctest, '--test-dir', str(build), '-j' + str(jobs),
                   '--output-on-failure', '--no-tests=error']
        before = resource.getrusage(resource.RUSAGE_CHILDREN)
        start = time.monotonic()
        with (output / 'ctest.log').open('w') as stream:
            result = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT)
        wall = time.monotonic() - start
        after = resource.getrusage(resource.RUSAGE_CHILDREN)
        missing = [t['name'] for t in inventory['tests']
                   if not (output / (t['name'] + '.json')).is_file()]
        summary = dict(command=command, jobs=jobs, profiling=profile,
                       test_count=len(inventory['tests']), wall_s=wall,
                       cpu_user_s=after.ru_utime - before.ru_utime,
                       cpu_system_s=after.ru_stime - before.ru_stime,
                       exit_code=result.returncode, missing_records=missing,
                       affinity=sorted(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else None)
        (output / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
        print(json.dumps(summary, indent=2))
        return result.returncode or (1 if missing else 0)
    finally:
        # Refuse to silently overwrite a concurrent reconfigure's generated file.
        if wrapped is not None and testfile.read_bytes() != wrapped:
            raise RuntimeError('Concurrent CMake modified CTest launch file; manual recovery required')
        if original is not None:
            testfile.write_bytes(original)
        lock.unlink()


def main():
    if len(sys.argv) > 1 and sys.argv[1] == '--_child':
        return timed_child(Path(sys.argv[2]), sys.argv[3], sys.argv[4] == '1', sys.argv[5:])
    if len(sys.argv) > 1 and sys.argv[1] == '--_observe':
        observe_python(Path(sys.argv[2]), sys.argv[3:])
        return 0
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--jobs', type=int, choices=[1, 2], required=True)
    p.add_argument('--profile', action='store_true', help='gprofng10ms samples in six expensive Debug cases')
    p.add_argument('--ctest', default='ctest')
    args = p.parse_args()
    if args.profile and not shutil.which('gprofng'):
        p.error('gprofng required for --profile')
    return benchmark(args.build, args.output, args.jobs, args.profile, args.ctest)


if __name__ == '__main__':
    sys.exit(main())
