"""The timing tool must preserve CTest commands, failures, isolation and bytes."""
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(sys.argv[1]).resolve()
spec = importlib.util.spec_from_file_location('sf_benchmark', TOOL)
benchmark = importlib.util.module_from_spec(spec)
spec.loader.exec_module(benchmark)


class BenchmarkContract(unittest.TestCase):
    def test_failure_is_measured_and_launch_file_restored(self):
        with tempfile.TemporaryDirectory(prefix='sf-benchmark-contract-') as d:
            root = Path(d)
            build = root / 'build'
            build.mkdir()
            launch = build / 'CTestTestfile.cmake'
            original = (f'add_test(ok "{sys.executable}" "-c" "print(42)")\n'
                        f'add_test(bad "{sys.executable}" "-c" "raise SystemExit(7)")\n').encode()
            launch.write_bytes(original)
            evidence = root / 'evidence'
            result = subprocess.run([sys.executable, str(TOOL), '--build', str(build),
                                     '--output', str(evidence), '--jobs', '2'],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 8, result.stdout + result.stderr)
            self.assertEqual(launch.read_bytes(), original)
            summary = json.loads((evidence / 'summary.json').read_text())
            self.assertEqual(summary['test_count'], 2)
            self.assertEqual(summary['missing_records'], [])
            self.assertEqual(json.loads((evidence / 'bad.json').read_text())['exit_code'], 7)
            self.assertEqual(json.loads((evidence / 'ok.json').read_text())['exit_code'], 0)
            self.assertFalse((build / '.soundforge-benchmark.lock').exists())
            # An old evidence directory cannot make a later run appear to pass.
            again = subprocess.run([sys.executable, str(TOOL), '--build', str(build),
                                    '--output', str(evidence), '--jobs', '1'],
                                   capture_output=True, text=True)
            self.assertNotEqual(again.returncode, 0)
            self.assertEqual(launch.read_bytes(), original)

    def test_live_build_lock_rejects_second_measurement(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            launch = root / 'CTestTestfile.cmake'
            launch.write_text('add_test(unused "false")\n')
            (root / '.soundforge-benchmark.lock').touch()
            result = subprocess.run([sys.executable, str(TOOL), '--build', str(root),
                                     '--output', str(root / 'out'), '--jobs', '1'],
                                    capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertTrue((root / '.soundforge-benchmark.lock').exists())
            self.assertEqual(launch.read_text(), 'add_test(unused "false")\n')

    def test_only_declared_nondeterministic_timing_is_ignored(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            pcm = root / 'sample.wav'
            pcm.write_bytes(b'actual byte-preservation sentinel')
            receipt = root / 'renders.csv'
            receipt.write_text('file,raw_rms,host_processor_us\na.wav,0.1,10\n')
            before = benchmark.artifact_hashes(root)
            receipt.write_text('file,raw_rms,host_processor_us\na.wav,0.1,999\n')
            self.assertEqual(benchmark.artifact_hashes(root), before)
            receipt.write_text('file,raw_rms,host_processor_us\na.wav,0.2,999\n')
            self.assertNotEqual(benchmark.artifact_hashes(root), before)
            receipt.write_text('file,raw_rms,host_processor_us\na.wav,0.1,10\n')
            pcm.write_bytes(b'changed audio must fail preservation')
            self.assertNotEqual(benchmark.artifact_hashes(root), before)


if __name__ == '__main__':
    if not shutil.which('ctest'):
        raise RuntimeError('CTest is required; do not silently skip this contract')
    unittest.main(argv=[sys.argv[0]])
