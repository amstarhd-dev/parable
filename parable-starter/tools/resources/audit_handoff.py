#!/usr/bin/env python3
"""Inspect actual linked benchmark ELF hooks; does not prove ROM execution."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile


def audit(root, objdump):
    root = Path(root)
    manifest = json.loads((root / 'artifacts.json').read_text())
    rows = []
    for a in manifest['artifacts']:
        binary = root / a['bin_file']
        compressed = binary.with_suffix('.elf.gz')
        elf = gzip.decompress(compressed.read_bytes())
        if hashlib.sha256(elf).hexdigest() != a['elf_sha256']:
            raise ValueError('ELF hash mismatch')
        with tempfile.NamedTemporaryFile(suffix='.elf') as temp:
            temp.write(elf)
            temp.flush()
            def dump(*args):
                return subprocess.check_output([str(objdump), *args, temp.name]).decode().replace(temp.name, binary.with_suffix('.elf').name)
            sections = dump('-h')
            reset = dump('-d', '--disassemble=Reset_Handler')
            hook = dump('-d', '--disassemble=__wrap_SystemInit')
            # Ensure actual startup calls hook before constructors; the wrapper
            # itself precedes hardware initialization and never calls libc/HAL.
            if reset.find('<__wrap_SystemInit>') < 0 or reset.find('<__libc_init_array>') < reset.find('<__wrap_SystemInit>'):
                raise ValueError('Wrong startup hook order')
            marker = re.search(r'^\s*\d+\s+\.parable_handoff\s+([0-9a-f]+)\s+([0-9a-f]+).*\n([^\n]+)', sections, re.M)
            if not marker or int(marker[1], 16) != 32 or int(marker[2], 16) % 32 or 'CONTENTS' in marker[3]:
                raise ValueError('Mailbox must be 32 aligned NOLOAD bytes')
            if not re.search(r'msr\s+MSP,', hook) or '<SystemInit>' not in hook:
                raise ValueError('Missing ROM stack switch / ordinary startup fallback')
            if re.search(r'\b(vldr|vstr|vadd|vsub|vmul|vdiv|vpush|vpop|vcvt)', hook):
                raise ValueError('Unexpected early FPU use')
            evidence = reset + '\n' + hook + '\n' + sections
            destination = binary.parent.parent / 'handoff-disassembly.txt'
            destination.write_text(evidence)
            rows.append(dict(profile=a['profile'], artifact_id=a['artifact_id'], elf_sha256=a['elf_sha256'], mailbox_bytes=32, mailbox_address=int(marker[2], 16), startup_calls_wrapped_systeminit_before_constructors=True, evidence_sha256=hashlib.sha256(evidence.encode()).hexdigest(), physical_transition_tested=False))
    result = dict(schema='parable.handoff-link-audit.v1', artifacts=rows, physical_transition_tested=False)
    (root / 'handoff-audit.json').write_text(json.dumps(result, indent=2) + '\n')
    print('Audited %d linked startup hooks; physical transition remains UNTESTED' % len(rows))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--kit', type=Path, required=True)
    p.add_argument('--objdump', type=Path, required=True)
    a = p.parse_args()
    audit(a.kit, a.objdump)
