"""Render production ePaper artwork with its actual bitmap fonts; no device needed."""
from pathlib import Path
import struct
import subprocess
import zlib
root = Path(__file__).resolve().parents[1]
output = root / 'outputs'
output.mkdir(exist_ok=True)
subprocess.run(['clang++', '-std=c++17', '-Wall', '-Wextra', '-pedantic',
                '-I', str(root / 'tools/native_gfx'),
                '-I', str(root / '.pio/libdeps/cores3/Adafruit GFX Library'),
                '-I', str(root / 'include'), str(root / 'tools/render_paper_preview.cpp'),
                '-o', str(output / 'render_paper_preview')], check=True)
subprocess.run([str(output / 'render_paper_preview')], cwd=root, check=True)
def chunk(kind, data):
    return struct.pack('!I', len(data)) + kind + data + struct.pack('!I', zlib.crc32(kind + data))
for name in ['certificate', 'reception']:
    pgm = output / f'epaper_{name}_v06.pgm'
    magic, size, maximum, raw = pgm.read_bytes().split(b'\n', 3)
    width, height = map(int, size.split())
    assert magic == b'P5' and maximum == b'255' and len(raw) == width * height
    rows = b''.join(b'\0' + raw[y * width:(y + 1) * width] for y in range(height))
    png = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('!2I5B', width, height, 8, 0, 0, 0, 0))
           + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))
    pgm.with_suffix('.png').write_bytes(png)
    print(pgm.with_suffix('.png'))
