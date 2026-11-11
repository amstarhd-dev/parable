#!/usr/bin/env python3
"""Independent journal envelope oracle. Run from any directory; no C++ calls."""
from pathlib import Path
import struct
import zlib
root = Path(__file__).resolve().parent
payload = (root.parent / 'patch-v1/rat-composite.canonical.json').read_bytes()
header = struct.pack('<8sHHIQIII24s', b'PARPSET\0', 1, 64, 2, 9,
                     len(payload), zlib.crc32(payload), 1, bytes(24))
assert len(header) == 60
header += struct.pack('<I', zlib.crc32(header))
(root / 'slot2-gen9.header.hex').write_text(header.hex() + '\n')
