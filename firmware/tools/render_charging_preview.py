"""Preview the production C++ charging artwork; requires Pillow and clang++."""
from pathlib import Path
import subprocess
import tempfile
from PIL import Image

root = Path(__file__).resolve().parents[1]
out = root / 'outputs' / 'charging_v011'
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory() as temporary:
    temp = Path(temporary)
    binary = temp / 'render'
    subprocess.run(['clang++', '-std=c++17', '-Wall', '-Wextra', '-pedantic',
                    '-I', str(root / 'include'),
                    str(root / 'tools/render_frequency_preview.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(temp), 'charging'], check=True)
    frames = [Image.open(temp / f'{phase}.ppm').convert('RGB') for phase in range(32)]
    frames[8].save(out / 'alembic.png')
    large = [frame.resize((448, 448), Image.Resampling.NEAREST) for frame in frames]
    large[0].save(out / 'alembic.gif', save_all=True, append_images=large[1:],
                  duration=150, loop=0, disposal=2)
print(f'Production artwork: {out}/alembic.gif (32 frames, 150 ms/frame)')
