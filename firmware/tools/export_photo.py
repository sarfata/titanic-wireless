"""Convert a serial `photo dump` log to PBM and PNG, without extra packages."""
import argparse
from pathlib import Path
import re
import struct
import zlib

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('log', type=Path)
p.add_argument('output', type=Path, help='Output path stem, preferably outputs/photos/name')
args = p.parse_args()
image = None
complete = None
for line in args.log.read_text().splitlines():
    if re.search(r'PHOTOHEX /photo-\d+\.pbm 15000$', line):
        image = bytearray()
    elif 'PHOTOHEX END' in line and image is not None:
        if len(image) != 15000:
            raise SystemExit('Incomplete photo dump')
        complete = bytes(image)
        image = None
    elif image is not None:
        match = re.search(r'PH ([0-9a-f]{4}) ([0-9a-f]+)$', line)
        if match:
            if int(match[1], 16) != len(image):
                raise SystemExit('Missing or out-of-order photo data')
            image.extend(bytes.fromhex(match[2]))
if complete is None:
    raise SystemExit('No complete PHOTOHEX dump found')
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.with_suffix('.pbm').write_bytes(b'P4\n400 300\n' + complete)
def chunk(kind, data):
    return struct.pack('!I', len(data)) + kind + data + struct.pack('!I', zlib.crc32(kind + data))
# PNG grayscale bits use 1=white; PBM uses 1=black.
rows = b''.join(b'\0' + bytes(b ^ 255 for b in complete[y*50:(y+1)*50]) for y in range(300))
args.output.with_suffix('.png').write_bytes(
    b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('!2I5B', 400, 300, 1, 0, 0, 0, 0))
    + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))
print(f'Saved {args.output}.pbm and .png')
