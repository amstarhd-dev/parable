#!/usr/bin/env python3
"""Seed3 benchmark runner, stdlib only, Python >=3.6 / macOS 10.13.

No device access on import. `probe` must succeed physically before `batch`.
"""
import argparse
import csv
import fcntl
import hashlib
import io
import json
import os
import platform
import plistlib
import re
import secrets
import select
import struct
import subprocess
import sys
import tempfile
import termios
import time
from pathlib import Path

import hardware

HANDOFF = 'reset-systeminit-v1'
FLASH_LINKER_SHA = 'd45452b0f9f5b722b16e3a8fc168a76e2a5022d46a4efd10054880dcc5a87e8d'
DFU_SHA = 'a200781d74555c0d9c4b379fc7d5872deed6e13bf3988fc9d6313210090a8f43'
PROFILES = ('rat rat_fir97 rat_fir97_fp32 rat_1x_cost_only cmos germanium muff ts808 klon '
            'blender fz1 tape_saturation spring leo_hall chain_scorpio_gemini_rooms_0').split()


class Stop(RuntimeError):
    pass


class WaitTimeout(Stop):
    pass


def require(ok, message):
    if not ok:
        raise Stop(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def atomic(path, data):
    """Persist a complete file before granting the device permission to reboot."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(prefix='.' + path.name, dir=str(path.parent))
    try:
        with os.fdopen(fd, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(tmp, str(path))
        directory = os.open(str(path.parent), os.O_RDONLY)
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    finally:
        if os.path.exists(tmp):
            os.unlink(tmp)


def json_bytes(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode('ascii')


def load_kit(path, expected):
    path = Path(path).resolve()
    raw = path.read_bytes()
    require(sha(raw) == expected, 'Manifest SHA256 mismatch; obtain the reviewed kit hash')
    manifest = json.loads(raw.decode('utf-8'))
    require(manifest.get('schema') == 'parable.resource-artifacts.v1', 'Wrong manifest schema')
    rows = manifest['artifacts']
    require(len(rows) == 15 and {r['profile'] for r in rows} == set(PROFILES), 'Expected exactly the 15 PR #83 profiles')
    require(len({r['artifact_id'] for r in rows}) == 15, 'Duplicate artifact identity')
    for row in rows:
        require(row.get('status') == 'linked', 'Unlinked artifact')
        config = row['configuration']
        require(sha(json.dumps(config, sort_keys=True).encode()) == row['artifact_id'], 'Configuration identity mismatch')
        for key in ['artifact_id', 'bin_sha256', 'source_set_sha256', 'parent_artifact_id']:
            require(bool(re.fullmatch('[0-9a-f]{64}', row.get(key, ''))), 'Missing/invalid ' + key)
        require(row.get('control_protocol') == 'parable.control.v1' and row.get('handoff') == HANDOFF, 'No reviewed CDC/reset protocol')
        require(row.get('layout') == config['layout'] == 'BOOT_NONE' and row.get('flash_origin') == 0x08000000 and row.get('flash_length') == 131072, 'Only official internal 128 KiB BOOT_NONE supported')
        require(config.get('linker_sha256') == FLASH_LINKER_SHA, 'Wrong pinned linker script')
        require(config['pins']['libdaisy']['commit'] == '1289a000bc29dae1bc0ed4129cfa2df40ef71574', 'Wrong SDK pin')
        require(config['mode'] == row['mode'] == 2 and config['capture_blocks'] == row['capture_blocks'] == 3000, 'Unexpected benchmark controls')
        require(config['execution'] == row['execution'] == 'foreground_kernel', 'This batch preserves the original foreground methodology')
        require(config['profile']['id'] == row['profile'], 'Profile configuration mismatch')
        source_hash = sha(json.dumps(config['source_sha256'], sort_keys=True).encode())
        require(source_hash == row['source_set_sha256'], 'DSP source closure mismatch')
        binary = (path.parent / row['bin_file']).resolve()
        require(path.parent in binary.parents, 'Binary path escapes kit')
        blob = binary.read_bytes()
        require(sha(blob) == row['bin_sha256'] and 8 < len(blob) <= 131072, 'Binary hash/size mismatch')
        sp, reset = struct.unpack('<II', blob[:8])
        require(0x20000000 < sp <= 0x20020000 and not sp % 8 and reset & 1 and 0x08000000 <= reset < 0x08000000 + len(blob), 'Invalid internal-flash reset vectors')
    return manifest


def dfu_devices(text):
    devices = {}
    for line in text.splitlines():
        if not line.startswith('Found DFU: [0483:df11]'):
            continue
        fields = dict(re.findall(r'(\w+)=("[^"]*"|[^,\s]+)', line))
        fields = {k: v.strip('"') for k, v in fields.items()}
        require(all(k in fields for k in ['path', 'serial', 'alt', 'name', 'intf', 'cfg']), 'Unrecognized dfu-util listing')
        require(re.fullmatch('[A-Za-z0-9]+', fields['serial']) is not None, 'Missing/unsafe DFU serial')
        key = (fields['path'], fields['serial'])
        devices.setdefault(key, []).append(fields)
    return devices


def choose_dfu(text, serial):
    devices = dfu_devices(text)
    require(len(devices) == 1, 'Require exactly one STM32 ROM DFU device; found ' + str(len(devices)))
    (path, found), alts = next(iter(devices.items()))
    require(found == serial, 'Wrong ROM DFU serial; refusing to flash')
    internal = [a for a in alts if a['alt'] == '0' and a['intf'] == '0' and a['cfg'] == '1']
    require(len(internal) == 1, 'Ambiguous internal flash interface')
    require(re.fullmatch(r'@Internal Flash\s*/0x08000000/0?1\*128Kg', internal[0]['name']) is not None, 'ROM does not advertise the expected 128 KiB internal FLASH layout')
    return dict(path=path, serial=serial, layout=internal[0]['name'])


def silicon(uid, chip, flash):
    require(len(uid) == 12 and len(chip) == 4 and len(flash) == 2, 'Short chip identity upload')
    words = struct.unpack('<III', uid)
    chip_id = struct.unpack('<I', chip)[0]
    require(chip_id & 0xfff == 0x450 and struct.unpack('<H', flash)[0] == 128, 'Not the expected H74/H75-family, 128 KiB silicon')
    require(any(words) and words != (0xffffffff,) * 3, 'Invalid silicon UID')
    return dict(uid=''.join('%08x' % v for v in words), chip_id=chip_id, flash_kib=128)


def cdc_paths(tree):
    """Walk IORegistry ancestry, supporting High Sierra's USB class layouts."""
    found = []
    def visit(node, identity):
        if isinstance(node, list):
            for item in node:
                visit(item, identity)
        elif isinstance(node, dict):
            identity = dict(identity)
            for key in ('idVendor', 'idProduct', 'USB Serial Number', 'locationID'):
                if key in node:
                    identity[key] = node[key]
            path = node.get('IOCalloutDevice')
            if path and identity.get('idVendor') == 0x0483 and identity.get('idProduct') == 0x5740:
                require(isinstance(path, str) and path.startswith('/dev/cu.'), 'Unexpected serial callout path')
                found.append(dict(identity, path=path))
            visit(node.get('IORegistryEntryChildren', []), identity)
    visit(tree, {})
    return found


class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        try:
            fcntl.ioctl(self.fd, termios.TIOCEXCL)
            attributes = termios.tcgetattr(self.fd)
            attributes[0] = attributes[1] = attributes[3] = 0
            attributes[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
            attributes[4] = attributes[5] = termios.B115200
            attributes[6][termios.VMIN] = attributes[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, attributes)
        except BaseException:
            os.close(self.fd)
            raise
        self.pending = b''

    def close(self):
        os.close(self.fd)

    def send(self, text):
        data = text.encode('ascii')
        end = time.monotonic() + 3
        while data:
            require(time.monotonic() < end, 'Serial write timeout')
            if select.select([], [self.fd], [], .2)[1]:
                data = data[os.write(self.fd, data):]

    def line(self, deadline):
        while b'\n' not in self.pending:
            if time.monotonic() >= deadline:
                raise WaitTimeout('CDC timeout; device did not produce a complete report/response')
            if select.select([self.fd], [], [], .2)[0]:
                data = os.read(self.fd, 4096)
                require(bool(data), 'CDC disconnected before complete response')
                self.pending += data
                require(len(self.pending) <= 16384, 'Unbounded CDC line')
        line, self.pending = self.pending.split(b'\n', 1)
        return line.decode('ascii', errors='strict').rstrip('\r') + '\n'


def capture(port, manifest, artifact, timeout):
    end = time.monotonic() + timeout
    lines = []
    while True:
        line = port.line(end)
        if line.startswith('PARABLE_RESOURCE_V1 '):
            require(not lines, 'Interrupted/overlapping resource capture')
            lines.append(line)
        elif lines:
            lines.append(line)
        require(sum(map(len, lines)) <= 8192, 'Oversized resource report')
        if lines and line == 'END_RESOURCE_CAPTURE\n':
            raw = ''.join(lines)
            result = hardware.check(raw, manifest)
            require(result['capture']['artifact_id'] == artifact['artifact_id'], 'CDC is running a different firmware artifact')
            return raw, result


def hello(port, artifact, expected, timeout):
    nonce = secrets.token_hex(16)
    end = time.monotonic() + timeout
    # The final report packet can reach the host just before foreground sees
    # TX completion and unlocks RX. Retry HELLO only, never ENTER_DFU or flash.
    for attempt in range(3):
        port.send('HELLO ' + nonce + '\n')
        until = min(end, time.monotonic() + timeout / 3)
        try:
            while True:
                line = port.line(until)
                if not line.startswith('PARABLE_CONTROL_V1 '):
                    continue
                pairs = re.findall(r'(\w+)=([^\s]+)', line)
                fields = dict(pairs)
                require(len(pairs) == len(fields), 'Duplicate control identity field')
                require(set(fields) == {'artifact_id', 'nonce', 'uid', 'chip_id', 'flash_kib', 'board', 'handoff'}, 'Malformed control identity')
                require(fields['artifact_id'] == artifact['artifact_id'] and fields['nonce'] == nonce and fields['board'] == 'seed3' and fields['handoff'] == HANDOFF, 'Control identity/challenge mismatch')
                require(fields['uid'] == expected['uid'] and int(fields['chip_id']) == expected['chip_id'] and int(fields['flash_kib']) == 128, 'CDC/ROM silicon identity mismatch')
                return nonce, fields
        except WaitTimeout:
            pass
    raise WaitTimeout('CDC HELLO timed out after three bounded attempts; no reset requested')


class Mac:
    def __init__(self, dfu, output, serial):
        require(sys.platform == 'darwin', 'Physical backend requires macOS; tests use simulated transports')
        self.dfu = Path(dfu).resolve()
        require(sha(self.dfu.read_bytes()) == DFU_SHA, 'dfu-util executable differs from pinned Arduino 0.11-arduino5 Intel binary')
        self.output, self.serial = Path(output), serial
        version = self.command([str(self.dfu), '--version'])
        require('dfu-util 0.11' in version, 'Verified dfu-util failed version check')
        self.sequence = 0

    def command(self, args, timeout=30):
        proc = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
        text = proc.stdout.decode('utf-8', errors='replace')
        require(proc.returncode == 0, 'Command failed (no flash retry): ' + repr(args) + '\n' + text)
        return text

    def wait_dfu(self, timeout):
        end = time.monotonic() + timeout
        last_error = 'device absent'
        while time.monotonic() < end:
            try:
                listing = self.command([str(self.dfu), '-l'], timeout=5)
            except (Stop, subprocess.TimeoutExpired) as error:
                last_error = str(error)
                time.sleep(.25)
                continue # enumeration retry only, never a write retry
            devices = dfu_devices(listing)
            if devices:
                return choose_dfu(listing, self.serial) # wrong/ambiguous device is fatal
            time.sleep(.25)
        raise Stop('ROM DFU did not re-enumerate. STOP; no automatic reflash. Recover using BOOT + RESET. Last enumeration error: ' + last_error)

    def selectors(self, device):
        return [str(self.dfu), '-d', '0483:df11', '-S', device['serial'], '-p', device['path'], '-a', '0']

    def identity(self, device):
        values = []
        # `force` ONLY on these read-only bounded uploads: these addresses are
        # outside the ROM's advertised flash map. Never used with -D / erase.
        for address, length in [(0x1ff1e800, 12), (0x5c001000, 4), (0x1ff1e880, 2)]:
            with tempfile.TemporaryDirectory() as temporary:
                path = Path(temporary) / 'identity.bin'
                self.command(self.selectors(device) + ['-s', '0x%x:%d:force' % (address, length), '-U', str(path)])
                values.append(path.read_bytes())
        result = silicon(*values)
        return result

    def flash(self, device, path, expected):
        # Reverify immediately before every write; no blind retry on write errors.
        current = self.wait_dfu(10)
        require(current == device and self.identity(current) == expected, 'Device changed before flash')
        log = self.command(self.selectors(current) + ['-s', '0x08000000:leave', '-D', str(path)], timeout=90)
        self.sequence += 1
        atomic(self.output / ('flash-%02d.log' % self.sequence), log.encode())

    def open_cdc(self, timeout):
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            proc = subprocess.run(['/usr/sbin/ioreg', '-a', '-l', '-p', 'IOService'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
            require(proc.returncode == 0, 'Cannot inspect macOS USB/serial registry')
            paths = cdc_paths(plistlib.loads(proc.stdout))
            require(len(paths) <= 1, 'Multiple Daisy CDC devices; disconnect unrelated boards')
            if paths:
                try:
                    return Serial(paths[0]['path'])
                except OSError:
                    pass # bounded retry for node creation/permissions race
            time.sleep(.25)
        raise Stop('No Daisy USB CDC device appeared; no further flash attempted')


def receipt_context(manifest_sha, device, identity):
    return dict(schema='parable.dfu-transition.v1', manifest_sha256=manifest_sha,
                handoff=HANDOFF, dfu_util_sha256=DFU_SHA, runner_sha256=sha(Path(__file__).read_bytes()), device=device, silicon=identity)


def results(output, rows, complete=False):
    atomic(output / 'results.json', json_bytes(dict(schema='parable.batch-results.v1', measurements=rows,
           production_admitted=False, full_15_image_acceptance=complete,
           limitation='Foreground kernel cost is not real-time audio callback utilization or ISR-inclusive WCET.')))
    stream = io.StringIO()
    keys = ['profile', 'artifact_id', 'execution', 'max_cycles', 'nominal_block_fraction', 'screen', 'numerical_failures']
    writer = csv.DictWriter(stream, fieldnames=keys, lineterminator='\n')
    writer.writeheader()
    display = []
    for row in rows:
        c = row['capture']
        entry = dict(profile=c['profile'], artifact_id=c['artifact_id'], execution=c['execution'], max_cycles=c['max_cycles'], nominal_block_fraction=row['observed_fraction_of_nominal_block'], screen=row['screen'], numerical_failures=c['native_failures'] + c['finite_output_failures'])
        writer.writerow(entry)
        display.append('| {profile} | {max_cycles} | {nominal_block_fraction:.4f} | {screen} |'.format(**entry))
    atomic(output / 'results.csv', stream.getvalue().encode())
    atomic(output / 'results.md', ('# Seed3 benchmark observations\n\nForeground kernel cost only; **not audio callback utilization**. No production admission.\n\n| Profile | Maximum cycles | Fraction of nominal 480,000-cycle block | Screen |\n| --- | ---: | ---: | --- |\n' + '\n'.join(display) + '\n').encode())


def execute(backend, manifest, root, manifest_sha, output, probe=False, receipt=None, timeout=900):
    output = Path(output)
    require(not output.exists(), 'Choose a new output directory; never overwrite a prior run')
    output.mkdir(parents=True)
    device = backend.wait_dfu(20)
    identity = backend.identity(device) # family + actual flash size + UID BEFORE writes
    context = receipt_context(manifest_sha, device, identity)
    if not probe:
        require(receipt is not None and all(receipt.get(k) == v for k, v in context.items())
                and receipt.get('physical_transition_observed') is True
                and receipt.get('probe_artifact_id') == next(a['artifact_id'] for a in manifest['artifacts'] if a['profile'] == 'rat_1x_cost_only'), 'Batch locked: first run the separate physical probe with this kit/device/tool')
    chosen = [next(a for a in manifest['artifacts'] if a['profile'] == 'rat_1x_cost_only')] if probe else manifest['artifacts']
    observations = []
    atomic(output / 'session.json', json_bytes(context))
    for artifact in chosen:
        label = artifact['profile'] + '-' + artifact['artifact_id'][:12]
        # Detect binary changes after initial kit verification, too.
        binary = Path(root) / artifact['bin_file']
        require(sha(binary.read_bytes()) == artifact['bin_sha256'], 'Binary changed during run')
        print('Flashing ' + label + '; waiting for frozen benchmark report', flush=True)
        backend.flash(device, binary, identity)
        port = backend.open_cdc(timeout)
        try:
            raw, result = capture(port, manifest, artifact, timeout)
            atomic(output / (label + '.txt'), raw.encode('ascii')) # BEFORE HELLO/ENTER_DFU
            nonce, control = hello(port, artifact, identity, 10)
            atomic(output / (label + '.control.json'), json_bytes(control))
            observations.append(result)
            results(output, observations)
            port.send('ENTER_DFU ' + artifact['artifact_id'] + ' ' + nonce + '\n')
            end = time.monotonic() + 10
            while port.line(end) != 'DFU_AUTHORIZED ' + nonce + '\n':
                pass
        finally:
            port.close()
        print('Saved ' + label + '; waiting for authorized ROM DFU return', flush=True)
        returned = backend.wait_dfu(25)
        require(returned == device and backend.identity(returned) == identity, 'Returned DFU device/identity differs; STOP')
        atomic(output / (label + '.transition.json'), json_bytes(dict(device=returned, silicon=identity, observed=True)))
    if probe:
        context.update(physical_transition_observed=True, probe_artifact_id=chosen[0]['artifact_id'], host=platform.platform())
        atomic(output / 'transition-receipt.json', json_bytes(context))
    else:
        # Written ONLY after all fifteen reports AND all fifteen DFU returns.
        results(output, observations, complete=True)
        atomic(output / 'batch-complete.json', json_bytes(dict(context, images=len(observations), all_transitions_observed=True)))
    return observations


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['verify', 'probe', 'batch'])
    parser.add_argument('--manifest', required=True, type=Path)
    parser.add_argument('--manifest-sha256', required=True)
    parser.add_argument('--dfu-util', type=Path)
    parser.add_argument('--serial')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--transition-receipt', type=Path)
    parser.add_argument('--report-timeout', type=int, default=900, help='bounded benchmark/CDC enumeration timeout in seconds (30..3600)')
    parser.add_argument('--authorize-dfu-reset', action='store_true', help='explicit consent to flash benchmark images and reset this device to ROM DFU')
    args = parser.parse_args()
    try:
        require(30 <= args.report_timeout <= 3600, 'Report timeout must be 30..3600 seconds')
        manifest = load_kit(args.manifest, args.manifest_sha256)
        if args.action == 'verify':
            print('Verified 15 images, configuration identities, binary hashes, vectors and official internal-flash bounds. No USB access.')
            return 0
        require(args.authorize_dfu_reset, 'Explicit --authorize-dfu-reset required; no device access performed')
        require(args.dfu_util and args.serial and args.output, 'Supply --dfu-util, --serial, --output')
        receipt = json.loads(args.transition_receipt.read_text()) if args.transition_receipt else None
        if args.action == 'batch':
            require(receipt is not None, 'Batch locked: supply a successful physical --transition-receipt')
        execute(Mac(args.dfu_util, args.output, args.serial), manifest, args.manifest.parent.resolve(), args.manifest_sha256, args.output, args.action == 'probe', receipt, args.report_timeout)
        print('Reports saved. Device left in ROM DFU. No production admission inferred.')
        return 0
    except (Stop, ValueError, KeyError, OSError, subprocess.SubprocessError) as error:
        print('STOP: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
