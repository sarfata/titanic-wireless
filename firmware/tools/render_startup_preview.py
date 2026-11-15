"""Render the exact production LCD intro artwork without hardware."""
from pathlib import Path
import subprocess
import tempfile
from PIL import Image

root = Path(__file__).resolve().parents[1]
out = root / 'outputs' / 'startup_v012'
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory() as temporary:
    temp = Path(temporary)
    binary = temp / 'render'
    subprocess.run(['clang++', '-std=c++17', '-Wall', '-Wextra', '-pedantic',
                    '-I', str(root / 'include'), str(root / 'tools/render_frequency_preview.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(temp), 'startup'], check=True)
    frames = [Image.open(temp / f'{phase}.ppm').convert('RGB') for phase in range(48)]
    frames[8].save(out / 'startup.png')
    large = [frame.resize((448, 448), Image.Resampling.NEAREST) for frame in frames]
    large[0].save(out / 'startup.gif', save_all=True, append_images=large[1:],
                  duration=100, loop=0, disposal=2)
print(f'Production artwork: {out}/startup.gif')
