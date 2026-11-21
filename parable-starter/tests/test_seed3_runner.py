"""Simulated USB/serial only: never counted as physical acceptance evidence."""
import copy
import hashlib
import gzip
import json
import os
import pty
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time
import unittest
import zipfile
from unittest import mock

HERE = Path(__file__).resolve().parents[1] / 'tools/resources'
sys.path.insert(0, str(HERE))
import seed3_runner as runner
import hardware

# Descriptor copied verbatim from the founder's physical Seed3 probe.
# Other listing fields (including serial/path) remain synthetic test values.
OBSERVED_DESCRIPTOR = '@Internal Flash /0x08000000/16*128Kg'
LISTING = 'Found DFU: [0483:df11] ver=0200, devnum=2, cfg=1, intf=0, path="1-2", alt=0, name="' + OBSERVED_DESCRIPTOR + '", serial="1234ABCD"\n'
IDENTITY = dict(uid='112233445566778899aabbcc', chip_id=0x10030450, flash_kib=128)


def fixture(root):
    artifacts = []
    blob = struct.pack('<II', 0x20020000, 0x08000009) + bytes(24)
    for i, profile in enumerate(runner.PROFILES):
        config = dict(linker_sha256=runner.FLASH_LINKER_SHA, layout='BOOT_NONE', mode=2, capture_blocks=3000,
                      profile={'id': profile}, execution='foreground_kernel',
                      pins={'libdaisy': {'commit': '1289a000bc29dae1bc0ed4129cfa2df40ef71574'}},
                      source_sha256={profile: 'b' * 64})
        artifact = dict(profile=profile, artifact_id=runner.sha(json.dumps(config, sort_keys=True).encode()),
                        configuration=config, source_set_sha256=runner.sha(json.dumps(config['source_sha256'], sort_keys=True).encode()),
                        parent_artifact_id='f' * 64, status='linked', execution='foreground_kernel',
                        stimulus='synthetic_mix', mode=2, route=0, reserved_sdram_bytes=0,
                        capture_blocks=3000, bin_sha256=runner.sha(blob), bin_file=profile + '.bin',
                        layout='BOOT_NONE', flash_origin=0x08000000, flash_length=131072,
                        handoff=runner.HANDOFF, control_protocol='parable.control.v1')
        (root / artifact['bin_file']).write_bytes(blob)
        artifacts.append(artifact)
    manifest = dict(schema='parable.resource-artifacts.v1', artifacts=artifacts)
    data = runner.json_bytes(manifest)
    (root / 'artifacts.json').write_bytes(data)
    return manifest, runner.sha(data)


def report(artifact):
    fields = dict(artifact_id=artifact['artifact_id'], profile=artifact['profile'],
        execution=artifact['execution'], stimulus=artifact['stimulus'], mode=2, route=0,
        core_hz_rcc=480000000, sample_rate=48000, block_frames=48, deadline_cycles=480000,
        counter_enabled=1, blocks=3000, valid_blocks=3000, max_cycles=235475,
        deadline_hits=0, invalid_intervals=0, frame_mismatches=0,
        source_set_sha256=artifact['source_set_sha256'], reserved_sdram_bytes=0,
        used_sdram_bytes=0, finite_output_failures=0, fpscr=0, fpdscr=0,
        icache_enabled=1, dcache_enabled=1, native_failures=0, native_evaluations=0,
        native_solves=0, native_max_iterations=0, native_containment=0)
    return 'PARABLE_RESOURCE_V1 ' + ' '.join('%s=%s' % pair for pair in fields.items()) + '\nEND_RESOURCE_CAPTURE\n'


class FakePort:
    def __init__(self, backend):
        self.backend = backend
        self.lines = report(backend.artifact).splitlines(True)
        self.closed = False

    def line(self, deadline):
        if not self.lines:
            raise runner.Stop('Simulated serial timeout')
        return self.lines.pop(0)

    def send(self, command):
        b = self.backend
        if command.startswith('HELLO '):
            self.nonce = command.split()[1]
            self.lines.append('PARABLE_CONTROL_V1 artifact_id=%s nonce=%s uid=%s chip_id=%d flash_kib=128 board=seed3 handoff=%s\n' % (b.artifact['artifact_id'], self.nonce, b.control_uid, IDENTITY['chip_id'], runner.HANDOFF))
        elif command.startswith('ENTER_DFU '):
            assert command == 'ENTER_DFU ' + b.artifact['artifact_id'] + ' ' + self.nonce + '\n'
            # Exercise the actual runner's durable-save-before-reboot ordering.
            captures = list(b.output.glob('*.txt'))
            assert any(p.read_text() == report(b.artifact) for p in captures)
            b.enter += 1
            if not b.no_ack:
                self.lines.append('DFU_AUTHORIZED ' + self.nonce + '\n')
            b.state = 'stuck' if b.fail_transition == b.enter else 'dfu'
        else:
            raise AssertionError('Unexpected command')

    def close(self):
        self.closed = True


class FakeBackend:
    def __init__(self, manifest, output):
        self.manifest, self.output = manifest, output
        self.device = runner.choose_dfu(LISTING, '1234ABCD')
        self.state = 'dfu'
        self.flashes = []
        self.enter = 0
        self.fail_transition = -1
        self.no_ack = False
        self.control_uid = IDENTITY['uid']
        self.identity_change = False

    def wait_dfu(self, timeout):
        runner.require(self.state == 'dfu', 'Simulated DFU enumeration timeout; recover BOOT/RESET')
        return self.device

    def identity(self, device):
        runner.require(not self.identity_change, 'Simulated changed silicon identity')
        return IDENTITY

    def flash(self, device, binary, expected):
        assert self.state == 'dfu' and expected == IDENTITY
        self.artifact = next(a for a in self.manifest['artifacts'] if a['bin_file'] == binary.name)
        self.flashes.append(self.artifact['profile'])
        self.state = 'app'

    def open_cdc(self, timeout):
        assert self.state == 'app'
        return FakePort(self)


class Runner(unittest.TestCase):
    def test_identity_set_address_failure_never_reaches_flash(self):
        addresses = ['0x1ff1e800:12:force', '0x5c001000:4:force', '0x1ff1e880:2:force']
        for failing in range(3):
            backend = runner.Mac.__new__(runner.Mac)
            backend.dfu = Path('/verified/dfu-util')
            device = runner.choose_dfu(LISTING, '1234ABCD')
            backend.wait_dfu = lambda timeout: device
            commands = []
            def command(args):
                commands.append(args)
                self.assertIn('-U', args)
                address = args[args.index('-s') + 1]
                self.assertEqual(address, addresses[len(commands) - 1])
                if len(commands) - 1 == failing:
                    raise runner.Stop('SET_ADDRESS not correctly executed: ' + address)
                Path(args[-1]).write_bytes(bytes([1]) * [12, 4, 2][len(commands) - 1])
            backend.command = command
            with self.assertRaisesRegex(runner.Stop, addresses[failing].replace(':', '.')):
                backend.flash(device, Path('/never-flashed.bin'), IDENTITY)
            self.assertEqual(len(commands), failing + 1)
            self.assertTrue(all('-D' not in cmd for cmd in commands))

    def test_manifest_hash_binary_vectors_layout_and_identity(self):
        with tempfile.TemporaryDirectory() as t:
            root = Path(t)
            manifest, digest = fixture(root)
            path = root / 'artifacts.json'
            self.assertEqual(runner.load_kit(path, digest), manifest)
            with self.assertRaises(runner.Stop):
                runner.load_kit(path, '0' * 64)
            for mutate in [lambda a: a.update(layout='BOOT_QSPI'),
                           lambda a: a.update(flash_length=262144),
                           lambda a: a.update(bin_file='../escape'),
                           lambda a: a.update(parent_artifact_id='bad'),
                           lambda a: a['configuration'].update(mode=0),
                           lambda a: a.update(source_set_sha256='0' * 64)]:
                bad = copy.deepcopy(manifest)
                mutate(bad['artifacts'][0])
                raw = runner.json_bytes(bad)
                path.write_bytes(raw)
                with self.assertRaises(runner.Stop):
                    runner.load_kit(path, runner.sha(raw))
            fixture(root)
            (root / 'rat.bin').write_bytes(b'corrupted')
            with self.assertRaises(runner.Stop):
                runner.load_kit(path, digest)

    def test_dfu_listing_and_silicon_fail_closed(self):
        self.assertEqual(runner.choose_dfu(LISTING, '1234ABCD')['path'], '1-2')
        for text, serial in [('', '1234ABCD'), (LISTING, 'other'),
                (LISTING + LISTING.replace('1-2', '1-3'), '1234ABCD'),
                (LISTING.replace('128Kg', '256Kg'), '1234ABCD')]:
            with self.assertRaises(runner.Stop):
                runner.choose_dfu(text, serial)
        uid = struct.pack('<III', 0x11223344, 0x55667788, 0x99aabbcc)
        self.assertEqual(runner.silicon(uid, struct.pack('<I', IDENTITY['chip_id']), b'\x80\x00'), IDENTITY)
        for u, c, f in [(bytes(12), struct.pack('<I', 0x450), b'\x80\x00'),
                        (uid, struct.pack('<I', 0x451), b'\x80\x00'),
                        (uid, struct.pack('<I', 0x450), b'\x00\x08'),
                        (uid[:-1], bytes(4), b'\x80\x00')]:
            with self.assertRaises(runner.Stop):
                runner.silicon(u, c, f)

    def test_observed_seed3_descriptor_is_not_capacity_evidence(self):
        device = runner.choose_dfu(LISTING, '1234ABCD')
        self.assertEqual(device['layout'], OBSERVED_DESCRIPTOR)
        for sectors in ('1', '01'):
            legacy = LISTING.replace('16*128Kg', sectors + '*128Kg')
            self.assertEqual(runner.choose_dfu(legacy, '1234ABCD')['serial'], device['serial'])
        for bad in ('17*128Kg', '0*128Kg', '16*64Kg', '16*128Ka',
                    '16*128Kg/01*128Kg'):
            with self.assertRaises(runner.Stop):
                runner.choose_dfu(LISTING.replace('16*128Kg', bad), '1234ABCD')
        with self.assertRaises(runner.Stop):
            runner.choose_dfu(LISTING.replace('0x08000000', '0x08020000'), '1234ABCD')
        # A shared ROM map never grants permission to write on 2 MiB silicon.
        backend = runner.Mac.__new__(runner.Mac)
        backend.wait_dfu = lambda timeout: device
        def reject_capacity(selected):
            return runner.silicon(struct.pack('<III', 1, 2, 3),
                                  struct.pack('<I', 0x450), struct.pack('<H', 2048))
        backend.identity = reject_capacity
        backend.command = mock.Mock()
        with self.assertRaises(runner.Stop):
            backend.flash(device, Path('/reviewed/image.bin'), IDENTITY)
        backend.command.assert_not_called()

    def test_binary_bound_remains_128k_despite_observed_rom_map(self):
        runner.choose_dfu(LISTING, '1234ABCD')
        with tempfile.TemporaryDirectory() as t:
            root = Path(t)
            manifest, _ = fixture(root)
            binary = root / 'rat.bin'
            binary.write_bytes(binary.read_bytes() + bytes(131073 - binary.stat().st_size))
            manifest['artifacts'][0]['bin_sha256'] = runner.sha(binary.read_bytes())
            raw = runner.json_bytes(manifest)
            (root / 'artifacts.json').write_bytes(raw)
            with self.assertRaisesRegex(runner.Stop, 'Binary hash/size mismatch'):
                runner.load_kit(root / 'artifacts.json', runner.sha(raw))

    def test_dynamic_high_sierra_registry_ancestry(self):
        tree = [{'idVendor': 0x483, 'idProduct': 0x5740, 'USB Serial Number': 'CDCID',
                 'IORegistryEntryChildren': [{'IORegistryEntryChildren': [{'IOCalloutDevice': '/dev/cu.surprising-name'}]}]}]
        self.assertEqual(runner.cdc_paths(tree)[0]['path'], '/dev/cu.surprising-name')
        tree[0]['idProduct'] = 0xdf11
        self.assertEqual(runner.cdc_paths(tree), [])

    def test_actual_serial_transport_buffers_fragments_and_writes_commands(self):
        master, slave = pty.openpty()
        name = os.ttyname(slave)
        os.close(slave)
        port = runner.Serial(name)
        try:
            os.write(master, b'PARABLE_RES')
            os.write(master, b'OURCE_V1 test\nsecond\n')
            self.assertEqual(port.line(time.monotonic() + 1), 'PARABLE_RESOURCE_V1 test\n')
            self.assertEqual(port.line(time.monotonic() + 1), 'second\n')
            port.send('HELLO bounded\n')
            self.assertEqual(os.read(master, 100), b'HELLO bounded\n')
        finally:
            port.close()
            os.close(master)

    def test_hello_retries_report_completion_race_without_reset(self):
        with tempfile.TemporaryDirectory() as t:
            backend, args = self.run_context(Path(t), probe=True)
            backend.artifact = args['manifest']['artifacts'][0]
            port = FakePort(backend)
            original = port.send
            attempts = []
            port.lines = []
            def send(command):
                attempts.append(command)
                if len(attempts) > 1:
                    original(command)
            port.send = send
            read = port.line
            def line(deadline):
                if not port.lines:
                    raise runner.WaitTimeout('simulated foreground unlock race')
                return read(deadline)
            port.line = line
            runner.hello(port, backend.artifact, IDENTITY, 3)
            self.assertEqual(len(attempts), 2)
            self.assertEqual(attempts[0], attempts[1])
            self.assertEqual(backend.enter, 0)

    def test_transient_enumeration_retry_and_wrong_device_no_retry(self):
        backend = runner.Mac.__new__(runner.Mac)
        backend.dfu, backend.serial = Path('/verified/dfu-util'), '1234ABCD'
        backend.command = mock.Mock(side_effect=[runner.Stop('USB enumeration race'), '', LISTING])
        with mock.patch.object(runner.time, 'sleep'):
            self.assertEqual(backend.wait_dfu(2)['serial'], '1234ABCD')
        self.assertEqual(backend.command.call_count, 3)
        backend.command = mock.Mock(return_value=LISTING.replace('1234ABCD', 'WRONG'))
        with self.assertRaises(runner.Stop):
            backend.wait_dfu(2)
        self.assertEqual(backend.command.call_count, 1)

    def test_identity_force_is_read_only_and_flash_is_never_retried(self):
        backend = runner.Mac.__new__(runner.Mac)
        backend.dfu, backend.serial = Path('/verified/dfu-util'), '1234ABCD'
        device = runner.choose_dfu(LISTING, '1234ABCD')
        calls = []
        def upload(args):
            calls.append(args)
            self.assertIn('-U', args)
            address = args[args.index('-s') + 1]
            value = {'0x1ff1e800:12:force': struct.pack('<III', 0x11223344, 0x55667788, 0x99aabbcc),
                     '0x5c001000:4:force': struct.pack('<I', IDENTITY['chip_id']),
                     '0x1ff1e880:2:force': b'\x80\x00'}[address]
            Path(args[-1]).write_bytes(value)
        backend.command = upload
        self.assertEqual(backend.identity(device), IDENTITY)
        self.assertEqual(len(calls), 3)
        backend.wait_dfu = lambda timeout: device
        backend.identity = lambda device: IDENTITY
        backend.command = mock.Mock(side_effect=runner.Stop('write failed'))
        with self.assertRaises(runner.Stop):
            backend.flash(device, Path('/reviewed/image.bin'), IDENTITY)
        self.assertEqual(backend.command.call_count, 1)
        self.assertNotIn('force', backend.command.call_args[0][0])

    def run_context(self, root, probe=False, receipt=True):
        manifest, digest = fixture(root)
        output = root / 'results'
        backend = FakeBackend(manifest, output)
        context = runner.receipt_context(digest, backend.device, IDENTITY)
        context['physical_transition_observed'] = True
        context['probe_artifact_id'] = next(a['artifact_id'] for a in manifest['artifacts'] if a['profile'] == 'rat_1x_cost_only')
        return backend, dict(manifest=manifest, root=root, manifest_sha=digest, output=output,
                             probe=probe, receipt=context if receipt else None)

    def test_separate_probe_then_all_15_state_transitions(self):
        with tempfile.TemporaryDirectory() as t:
            backend, args = self.run_context(Path(t), probe=True, receipt=False)
            runner.execute(backend, **args)
            receipt = json.loads((args['output'] / 'transition-receipt.json').read_text())
            self.assertEqual(backend.flashes, ['rat_1x_cost_only'])
            self.assertTrue(receipt['physical_transition_observed'])
            backend.output = args['output'] = Path(t) / 'batch'
            args.update(probe=False, receipt=receipt)
            rows = runner.execute(backend, **args)
            self.assertEqual(len(rows), 15)
            self.assertEqual(backend.enter, 16)
            self.assertTrue((args['output'] / 'batch-complete.json').exists())
            self.assertEqual(len(list(args['output'].glob('*.txt'))), 15)
            self.assertTrue(all(r['production_admitted'] is False and r['screen'] == 'FOREGROUND_COST_ONLY' for r in rows))
            self.assertIn('not audio callback utilization', (args['output'] / 'results.md').read_text())

    def test_locked_batch_and_stale_receipts_never_flash(self):
        for change in [None, 'manifest_sha256', 'handoff', 'dfu_util_sha256', 'runner_sha256', 'probe_artifact_id', 'silicon']:
            with tempfile.TemporaryDirectory() as t:
                backend, args = self.run_context(Path(t), receipt=change is not None)
                if change:
                    args['receipt'][change] = 'wrong'
                with self.assertRaises(runner.Stop):
                    runner.execute(backend, **args)
                self.assertEqual(backend.flashes, [])

    def test_failed_final_transition_never_claims_acceptance(self):
        with tempfile.TemporaryDirectory() as t:
            backend, args = self.run_context(Path(t))
            backend.fail_transition = 15
            with self.assertRaises(runner.Stop):
                runner.execute(backend, **args)
            self.assertEqual(len(backend.flashes), 15)
            self.assertFalse((args['output'] / 'batch-complete.json').exists())
            self.assertFalse(json.loads((args['output'] / 'results.json').read_text())['full_15_image_acceptance'])

    def test_timeout_wrong_cdc_missing_ack_stop_without_reflash(self):
        for fault in ['fail_transition', 'control_uid', 'no_ack', 'identity_change']:
            with tempfile.TemporaryDirectory() as t:
                backend, args = self.run_context(Path(t), probe=True)
                setattr(backend, fault, {'fail_transition': 1, 'control_uid': '0' * 24, 'no_ack': True, 'identity_change': True}[fault])
                with self.assertRaises(runner.Stop):
                    runner.execute(backend, **args)
                self.assertLessEqual(len(backend.flashes), 1)
                self.assertFalse((args['output'] / 'transition-receipt.json').exists())

    def test_durable_raw_save_failure_prevents_reset(self):
        with tempfile.TemporaryDirectory() as t:
            backend, args = self.run_context(Path(t), probe=True)
            save = runner.atomic
            def fail_raw(path, data):
                if str(path).endswith('.txt'):
                    raise OSError('simulated disk full')
                save(path, data)
            with mock.patch.object(runner, 'atomic', fail_raw):
                with self.assertRaises(OSError):
                    runner.execute(backend, **args)
            self.assertEqual(backend.enter, 0)

    def test_malformed_partial_wrong_capture(self):
        with tempfile.TemporaryDirectory() as t:
            manifest, digest = fixture(Path(t))
            artifact = manifest['artifacts'][0]
            for text in [report(artifact).replace('END_RESOURCE_CAPTURE\n', ''),
                         report(manifest['artifacts'][1]),
                         report(artifact).replace('max_cycles=235475', 'max_cycles=0')]:
                port = mock.Mock()
                lines = iter(text.splitlines(True))
                def line(deadline):
                    try:
                        return next(lines)
                    except StopIteration:
                        raise runner.Stop('Timeout')
                port.line = line
                with self.assertRaises((runner.Stop, ValueError)):
                    runner.capture(port, manifest, artifact, 1)

    def test_actual_firmware_parser_sanitized(self):
        with tempfile.TemporaryDirectory() as t:
            binary = str(Path(t) / 'protocol')
            subprocess.run(['c++', '-std=c++17', '-O1', '-g', '-fsanitize=address,undefined',
                '-fno-omit-frame-pointer', '-fno-pie', '-no-pie', str(HERE / 'dfu_control_test.cpp'), '-o', binary], check=True)
            subprocess.run([binary], check=True)

    def test_python36_syntax_and_no_linux_only_imports(self):
        import ast
        source = (HERE / 'seed3_runner.py').read_text()
        ast.parse(source, feature_version=(3, 6))
        self.assertNotIn('import resource', source)

    def test_actual_download_kit_and_compiled_source_bindings(self):
        evidence = HERE.parents[1] / 'docs/resources/runner'
        receipt = json.loads((evidence / 'kit.json').read_text())
        blob = (evidence / receipt['file']).read_bytes()
        self.assertEqual(runner.sha(blob), receipt['sha256'])
        self.assertFalse(receipt['physical_acceptance_tested'])
        import io
        with zipfile.ZipFile(io.BytesIO(blob)) as archive, tempfile.TemporaryDirectory() as t:
            for line in archive.read('SHA256SUMS').decode().splitlines():
                digest, name = line.split('  ', 1)
                self.assertEqual(runner.sha(archive.read(name)), digest, name)
            self.assertEqual(archive.read('seed3_runner.py'), (HERE / 'seed3_runner.py').read_bytes())
            release = json.loads(archive.read('runner-manifest.json'))
            self.assertEqual(release['version'], receipt['version'])
            self.assertEqual(release['runner_sha256'], runner.sha(archive.read('seed3_runner.py')))
            self.assertEqual(release['firmware_manifest_sha256'], receipt['manifest_sha256'])
            self.assertFalse(release['physical_rom_handoff_passed'])
            self.assertNotIn('transition-receipt.json', archive.namelist())
            raw = archive.read('artifacts.json')
            manifest = json.loads(raw)
            root = Path(t)
            (root / 'artifacts.json').write_bytes(raw)
            for a in manifest['artifacts']:
                binary = root / a['bin_file']
                binary.parent.mkdir(parents=True, exist_ok=True)
                binary.write_bytes(archive.read(a['bin_file']))
                elf = archive.read(str(Path(a['bin_file']).with_suffix('.elf.gz')))
                self.assertEqual(runner.sha(gzip.decompress(elf)), a['elf_sha256'])
                for name, digest in a['configuration']['harness_sha256'].items():
                    source = HERE / name if name != 'runtime_measurement.h' else HERE.parents[1] / 'firmware' / name
                    self.assertEqual(runner.sha(source.read_bytes()), digest)
            runner.load_kit(root / 'artifacts.json', receipt['manifest_sha256'])
            old = json.loads((evidence.parent / 'artifacts.json').read_text())
            parents = {a['artifact_id']: a for a in old['artifacts']}
            for a in manifest['artifacts']:
                self.assertEqual(a['configuration']['source_sha256'], parents[a['parent_artifact_id']]['configuration']['source_sha256'])


if __name__ == '__main__':
    unittest.main()
