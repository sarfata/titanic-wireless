"""Build frequency PNG examples and a browser atlas from production C++ artwork."""
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

root = Path(__file__).resolve().parents[1]
output = root / 'outputs'
output.mkdir(exist_ok=True)
binary = output / 'render_frequency_preview'
subprocess.run(['clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-pedantic',
                '-I', str(root / 'include'), str(root / 'tools/render_frequency_preview.cpp'),
                '-o', str(binary)], check=True)

def png(path, width, height, pixels):
    def chunk(kind, data):
        return struct.pack('!I', len(data)) + kind + data + struct.pack('!I', zlib.crc32(kind + data))
    rows = b''.join(b'\0' + pixels[y * width * 3:(y + 1) * width * 3] for y in range(height))
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('!2I5B', width, height, 8, 2, 0, 0, 0))
                     + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))

with tempfile.TemporaryDirectory() as directory:
    subprocess.run([str(binary), directory], check=True)
    width, height = 2240, 2464  # 10 columns, 11 rows; every integer 450..550.
    atlas = bytearray(width * height * 3)
    for frequency in range(450, 551):
        header, size, maximum, pixels = (Path(directory) / f'{frequency}.ppm').read_bytes().split(b'\n', 3)
        assert header == b'P6' and size == b'224 224' and maximum == b'255'
        assert len(pixels) == 224 * 224 * 3
        i = frequency - 450
        for y in range(224):
            start = ((i // 10 * 224 + y) * width + i % 10 * 224) * 3
            atlas[start:start + 224 * 3] = pixels[y * 224 * 3:(y + 1) * 224 * 3]
        if frequency in (450, 470, 500, 530, 550):
            png(output / f'frequency_v07_{frequency}.png', 224, 224, pixels)
    png(output / 'frequency_atlas_v07.png', width, height, atlas)
    atlas = bytearray(width * height * 3)
    for volume in range(101):
        pixels = (Path(directory) / f'volume-{volume}.ppm').read_bytes().split(b'\n', 3)[3]
        for y in range(224):
            start = ((volume // 10 * 224 + y) * width + volume % 10 * 224) * 3
            atlas[start:start + 224 * 3] = pixels[y * 224 * 3:(y + 1) * 224 * 3]
        if volume in (0, 50, 80, 100):
            png(output / f'volume_v08_{volume}.png', 224, 224, pixels)
    png(output / 'volume_atlas_v08.png', width, height, atlas)
print('Rendered frequency_v07_{450,470,500,530,550}.png and frequency_atlas_v07.png')
