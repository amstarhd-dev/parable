#!/usr/bin/env python3
"""One-BOOT Seed3 15-image benchmark session using ROM USB identity + CDC silicon identity.

This explicit experimental path does NOT read undocumented ROM DFU register addresses.
The first flash is allowed only with --authorize-first-flash-without-rom-silicon-read.
After flashing, the actual firmware must attest native UID/chip ID/flash size over
nonce-bound CDC before any next flash, and every later image must match that UID.
All ROM transitions and reports must succeed; no automatic flash retry.
"""
import argparse
import json
import platform
import re
import secrets
import subprocess
import sys
import time
from pathlib import Path
import seed3_runner as r

def cdc_identity(port, artifact, timeout=15):
    """Challenge the flashed application and derive *post-flash* silicon identity."""
    nonce = secrets.token_hex(16)
    end = time.monotonic() + timeout
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
                r.require(len(fields) == len(pairs) and
                          set(fields) == {'artifact_id','nonce','uid','chip_id',
                                          'flash_kib','board','handoff'},
                          'Malformed or ambiguous CDC identity')
                r.require(fields['artifact_id'] == artifact['artifact_id'] and
                          fields['nonce'] == nonce and fields['board'] == 'seed3' and
                          fields['handoff'] == r.HANDOFF,
                          'Wrong CDC artifact, nonce, board, or handoff protocol')
                uid = fields['uid']
                r.require(re.fullmatch('[0-9a-f]{24}', uid) is not None and
                          uid not in ('0'*24, 'f'*24),
                          'Invalid native UID reported by firmware')
                chip = int(fields['chip_id'])
                flash = int(fields['flash_kib'])
                r.require(chip & 0xfff == 0x450 and flash == 128,
                          'Firmware reports wrong chip family or flash capacity')
                return nonce, fields, dict(uid=uid, chip_id=chip, flash_kib=flash)
        except r.WaitTimeout:
            pass
    raise r.WaitTimeout('CDC identity challenge timed out; STOP without further flash')

class Device(r.Mac):
    def flash_selected(self, selected, binary):
        current = self.wait_dfu(10)
        r.require(current == selected, 'ROM DFU device changed before flashing')
        args = self.selectors(current) + ['-s','0x08000000:leave','-D',str(binary)]
        self.sequence += 1
        # Never retry a failed write or infer success from USB disconnection.
        output = self.command(args, timeout=90)
        r.atomic(self.output / ('flash-%02d.log' % self.sequence), output.encode('utf-8'))

def batch(backend, manifest, manifest_hash, kit, output, timeout):
    output = Path(output)
    r.require(not output.exists(), 'Output directory already exists; choose a new one')
    output.mkdir(parents=True)
    selected = backend.wait_dfu(20)
    # ROM exposes device USB serial/path, NOT the H750 chip ID or factory size.
    r.atomic(output/'session.json', r.json_bytes(dict(
        schema='parable.one-boot-batch.v1', manifest_sha256=manifest_hash,
        dfu_serial=selected['serial'], dfu_path=selected['path'],
        initial_silicon_read='NOT AVAILABLE THROUGH ROM DFU',
        first_flash_requires_explicit_authorization=True,
        flash_programming_permitted_only_for_verified_128k_images=True,
        physical_acceptance=False)))
    primary = next(a for a in manifest['artifacts'] if a['profile'] == 'rat_1x_cost_only')
    queue = [primary] + [a for a in manifest['artifacts'] if a['artifact_id'] != primary['artifact_id']]
    silicon = None
    observations = []
    for index,artifact in enumerate(queue,1):
        binary = (kit / artifact['bin_file']).resolve()
        r.require(kit in binary.parents and r.sha(binary.read_bytes()) == artifact['bin_sha256'],
                  'Binary changed after manifest verification')
        r.require(binary.stat().st_size <= 131072,
                  'Cannot program beyond physical 128KiB flash')
        label = '%02d-%s-%s' % (index,artifact['profile'],artifact['artifact_id'][:12])
        print('Flashing %d/15: %s' % (index,artifact['profile']), flush=True)
        backend.flash_selected(selected,binary)
        port = backend.open_cdc(timeout)
        try:
            raw,result = r.capture(port,manifest,artifact,timeout)
            r.atomic(output/(label+'.txt'),raw.encode('ascii'))
            nonce,control,observed = cdc_identity(port,artifact)
            if silicon is None:
                silicon = observed
                r.atomic(output/'first-native-silicon.json',r.json_bytes(silicon))
            else:
                r.require(observed == silicon,
                          'Changed silicon UID/chip/capacity; STOP without further flash')
            r.atomic(output/(label+'.control.json'),r.json_bytes(control))
            observations.append(result)
            r.results(output,observations,complete=False)
            port.send('ENTER_DFU ' + artifact['artifact_id'] + ' ' + nonce + '\n')
            end=time.monotonic()+10
            while port.line(end) != 'DFU_AUTHORIZED ' + nonce + '\n':
                pass
        finally:
            port.close()
        print('Saved %d/15; verifying automatic ROM DFU return' % index, flush=True)
        returned=backend.wait_dfu(25)
        r.require(returned==selected,'ROM DFU serial/path/interface changed; STOP')
        r.atomic(output/(label+'.transition.json'),r.json_bytes(dict(
            rom_return_observed=True,rom_usb_identity=returned,
            native_silicon_identity=observed,identity_source='firmware_CDC_post_flash')))
    r.results(output,observations,complete=True)
    r.atomic(output/'batch-complete.json',r.json_bytes(dict(
        schema='parable.one-boot-acceptance.v1',image_count=15,
        rom_returns=15,firmware_native_silicon=silicon,
        identity_preflash_first_image=False,
        tested_execution='foreground_kernel',
        limitation='Not audio callback utilization; first image was selected by ROM USB descriptor only.')))
    print('Completed 15 reports and 15 observed automatic ROM returns.',flush=True)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--manifest',required=True,type=Path)
    p.add_argument('--manifest-sha256',required=True)
    p.add_argument('--dfu-util',required=True,type=Path)
    p.add_argument('--serial',required=True)
    p.add_argument('--output',required=True,type=Path)
    p.add_argument('--report-timeout',type=int,default=180)
    p.add_argument('--authorize-first-flash-without-rom-silicon-read',action='store_true')
    a=p.parse_args()
    try:
        r.require(a.authorize_first_flash_without_rom_silicon_read,
                  'No supported ROM DFU silicon read. Explicit first-flash authorization required')
        r.require(30<=a.report_timeout<=3600,'Report timeout must be 30..3600s')
        manifest=r.load_kit(a.manifest,a.manifest_sha256)
        backend=Device(a.dfu_util,a.output,a.serial)
        batch(backend,manifest,a.manifest_sha256,a.manifest.parent.resolve(),a.output,a.report_timeout)
        return 0
    except (r.Stop,ValueError,KeyError,OSError,subprocess.SubprocessError) as error:
        print('STOP: '+str(error),file=sys.stderr)
        return 1
if __name__=='__main__':
    sys.exit(main())
