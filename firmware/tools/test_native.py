"""Run hardware-independent tests without leaving executables in the repository."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as directory:
    for source in sorted((root / 'test').glob('*_test.cpp')):
        binary = Path(directory) / source.stem
        subprocess.run(['clang++', '-std=c++17', '-Wall', '-Wextra', '-pedantic',
                        '-I', str(root / 'include'), str(source), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
