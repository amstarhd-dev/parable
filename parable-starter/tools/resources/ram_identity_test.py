#!/usr/bin/env python3
"""Separate RAM-only Seed3 identity test. Python3.6+, stdlib. NO flash command."""
import argparse
import hashlib
import json
import secrets
import struct
import subprocess
import sys
import tempfile
import time
import zlib
from pathlib import Path
import seed3_runner as common

BASE, LIMIT, MAIL, SIZE = 0x24040000, 4096, 0x24041000, 128
REQUEST, RESPONSE = 0x50494451, 0x50494452

def validate_kit(path, expected):
    path = Path(path).resolve()
    raw = path.read_bytes()
    common.require(common.sha(raw) == expected, 'RAM manifest hash mismatch')
    m = json.loads(raw.decode())
    common.require(m['schema'] == 'parable.ram-identity.v1' and m['flash_programming_supported'] is False, 'Wrong RAM instrument manifest')
    common.require((m['code_address'], m['code_limit'], m['mailbox_address'], m['mailbox_bytes'], m['stack_bottom'], m['stack_top']) == (BASE, LIMIT, MAIL, SIZE, 0x2001fc00, 0x20020000), 'RAM layout mismatch')
    common.require(m['artifact_id'] == common.sha(json.dumps(m['configuration'], sort_keys=True).encode()), 'RAM artifact identity mismatch')
    common.require(m['configuration']['pins']['libdaisy']['commit'] == '1289a000bc29dae1bc0ed4129cfa2df40ef71574', 'Wrong pinned SDK')
    common.require(m['bin_file'] == 'ram-identity.bin', 'Unexpected executable filename')
    blob = (path.parent / m['bin_file']).read_bytes()
    common.require(len(blob) == m['bin_bytes'] and 664 <= len(blob) <= LIMIT and common.sha(blob) == m['bin_sha256'], 'RAM executable hash/size mismatch')
    sp, entry = struct.unpack('<II', blob[:8])
    common.require(sp == 0x20020000 and entry & 1 and BASE <= entry < BASE + len(blob), 'RAM vector mismatch')
    return m, blob

def request(artifact, nonce):
    common.require(len(nonce) == 16 and any(nonce), 'Invalid challenge')
    return struct.pack('<4I', REQUEST, 1, 32, 0) + nonce + bytes.fromhex(artifact) + bytes(64)

def response(raw, manifest, nonce):
    common.require(len(raw) == SIZE, 'Short RAM identity response')
    w = struct.unpack('<32I', raw)
    common.require(w[:4] == (RESPONSE, 1, 32, 0), 'RAM helper did not commit a successful response')
    common.require(raw[16:32] == nonce and raw[32:64] == bytes.fromhex(manifest['artifact_id']), 'Stale/wrong nonce or helper identity')
    common.require(w[31] == zlib.crc32(raw[4:124]) & 0xffffffff, 'RAM response CRC mismatch')
    common.require(w[24] == BASE and w[25] == 3 and w[26] <= 511 and w[27] == 0 and w[28] == 1 and not any(w[29:31]), 'Unexpected startup/return protocol')
    common.require(w[20] == 128, 'Factory flash capacity must remain 128 KiB')
    identity = common.silicon(raw[64:76], raw[76:80], struct.pack('<H', w[20]))
    return dict(identity=identity, reset_flags=w[21], entry_ccr=w[22], entry_vtor=w[23], helper_vtor=w[24], entry_ipsr=w[26], control=w[27], primask=w[28])

class RamDfu:
    def __init__(self, dfu, serial, output):
        self.usb = common.Mac(dfu, output, serial) # executable hash/version check only
        self.output = Path(output)
        self.command_number = 0

    def command(self, device, address, length=None, download=None, leave=False):
        # Only these exact volatile regions and operations can reach dfu-util.
        common.require(address in (BASE, MAIL), 'RAM-only transport refuses this address')
        common.require(not (leave and (length is not None or download is not None)) and (leave or (length is None) != (download is None)), 'Ambiguous RAM operation')
        if leave:
            common.require(address == BASE, 'Only the verified RAM entry may execute')
        elif download is not None:
            data = Path(download).read_bytes()
            common.require(0 < len(data) <= (LIMIT if address == BASE else SIZE), 'RAM write out of bounds')
            common.require(address != MAIL or len(data) == SIZE, 'Mailbox write must be exactly128 bytes')
        else:
            common.require(isinstance(length, int) and 0 < length <= (LIMIT if address == BASE else SIZE), 'RAM read out of bounds')
        selected = self.usb.wait_dfu(10)
        common.require(selected == device, 'USB device changed before RAM operation')
        args = list(self.usb.selectors(device))
        if leave:
            args += ['-s', '0x%x:leave' % address] # DfuSe command mode, no -D
        elif download is not None:
            args += ['-s', '0x%x:force' % address, '-D', str(download)]
        else:
            args += ['-s', '0x%x:%d:force' % (address, length), '-U', str(self.output / 'upload.tmp')]
        self.command_number += 1
        # Save command and combined output even on a failing SET_ADDRESS.
        atomic = common.atomic
        atomic(self.output / ('command-%02d.json' % self.command_number), common.json_bytes(args))
        try:
            log = self.usb.command(args, timeout=30)
        except (common.Stop, subprocess.SubprocessError) as error:
            atomic(self.output / ('command-%02d-error.txt' % self.command_number), str(error).encode())
            raise
        atomic(self.output / ('command-%02d.txt' % self.command_number), log.encode())
        if length is not None:
            path = self.output / 'upload.tmp'
            raw = path.read_bytes()
            path.unlink()
            common.require(len(raw) == length, 'Short ROM RAM upload')
            return raw

    def enumeration(self, device):
        text = self.usb.command([str(self.usb.dfu), '-l'], timeout=5)
        selected = common.choose_dfu(text, device['serial'])
        common.require(selected == device, 'Device changed before RAM execution')
        fields = common.dfu_devices(text)[(device['path'], device['serial'])]
        token = {f.get('devnum') for f in fields}
        common.require(len(token) == 1 and None not in token, 'Cannot track ROM enumeration generation')
        return next(iter(token))

    def returned(self, device, previous):
        end = time.monotonic() + 25
        absent = False
        while time.monotonic() < end:
            text = self.usb.command([str(self.usb.dfu), '-l'], timeout=5)
            devices = common.dfu_devices(text)
            if not devices:
                absent = True
            else:
                common.require(common.choose_dfu(text, device['serial']) == device, 'Different ROM device returned')
                fields = devices[(device['path'], device['serial'])]
                token = {f.get('devnum') for f in fields}
                common.require(len(token) == 1 and None not in token, 'Unrecognized return enumeration')
                if absent or next(iter(token)) != previous:
                    return
            time.sleep(.1)
        raise common.Stop('No verified new ROM enumeration after RAM execution. STOP. Use ordinary BOOT/RESET recovery; do not retry automatically.')

def execute(transport, manifest, blob, output):
    output = Path(output)
    device = transport.usb.wait_dfu(20)
    common.atomic(output / 'session.json', common.json_bytes(dict(artifact_id=manifest['artifact_id'], bin_sha256=manifest['bin_sha256'], device=device, flash_programming_permitted=False)))
    observations = []
    for round_number in (1, 2):
        print('Round %d/2: downloading and verifying volatile RAM only' % round_number, flush=True)
        nonce = secrets.token_bytes(16)
        challenge = request(manifest['artifact_id'], nonce)
        common.atomic(output / ('round-%d.request' % round_number), challenge)
        with tempfile.TemporaryDirectory() as temporary:
            image = Path(temporary) / 'ram.bin'; image.write_bytes(blob)
            mailbox = Path(temporary) / 'request.bin'; mailbox.write_bytes(challenge)
            transport.command(device, BASE, download=image)
            common.require(transport.command(device, BASE, length=len(blob)) == blob, 'Executable RAM readback differs; will not execute')
            transport.command(device, MAIL, download=mailbox)
            common.require(transport.command(device, MAIL, length=SIZE) == challenge, 'Challenge RAM readback differs; will not execute')
        generation = transport.enumeration(device)
        print('Round %d/2: executing RAM helper; waiting for ROM return' % round_number, flush=True)
        transport.command(device, BASE, leave=True)
        transport.returned(device, generation)
        raw = transport.command(device, MAIL, length=SIZE)
        common.atomic(output / ('round-%d.raw' % round_number), raw)
        parsed = response(raw, manifest, nonce)
        common.atomic(output / ('round-%d.json' % round_number), common.json_bytes(parsed))
        observations.append(parsed)
        if round_number == 2:
            common.require(observations[0]['identity'] == observations[1]['identity'], 'Native silicon identity changed across ROM return')
    # Deliberately NOT a full runner transition receipt. No batch admission.
    result = dict(schema='parable.ram-identity-observation.v1', artifact_id=manifest['artifact_id'], device=device, rounds=observations, rom_returns_observed=2, flash_programming_permitted=False, full_runner_integrated=False)
    common.atomic(output / 'identity-observation.json', common.json_bytes(result))
    return result

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--manifest', required=True, type=Path)
    p.add_argument('--manifest-sha256', required=True)
    p.add_argument('--dfu-util', required=True, type=Path)
    p.add_argument('--serial', required=True)
    p.add_argument('--output', required=True, type=Path)
    p.add_argument('--authorize-seed3-pg3-ram-test', action='store_true', help='consent to RAM downloads and Seed3 PG3 BOOT drive/reset; no flash writes')
    a = p.parse_args()
    try:
        common.require(a.authorize_seed3_pg3_ram_test, 'Explicit Seed3 PG3/RAM/reset authorization required')
        manifest, blob = validate_kit(a.manifest, a.manifest_sha256)
        common.require(not a.output.exists(), 'Use a new output directory')
        a.output.mkdir(parents=True)
        result = execute(RamDfu(a.dfu_util, a.serial, a.output), manifest, blob, a.output)
        print('Two native identity/ROM-return observations saved; NO permission to flash or run the benchmark batch.')
        return 0
    except (common.Stop, OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as error:
        print('STOP: ' + str(error), file=sys.stderr)
        return 1
if __name__ == '__main__':
    sys.exit(main())
