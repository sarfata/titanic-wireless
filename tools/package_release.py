"""Package only current printable parts and their assembly documentation."""
from pathlib import Path
import hashlib
import zipfile

root = Path(__file__).resolve().parents[1]
dest = root / 'dist'
dest.mkdir(exist_ok=True)
archive = dest / 'Titanic-Wireless-Print-Kit-1.0.zip'
folders = {
    'bottom_v11': '01-bottom-v1.1',
    'front_v11': '02-front-v1.1',
    'tuning_knob_v02': '03-tuning-knob-v0.2',
    'top_button_key_v01': '04-top-button-and-optional-key-v0.1',
    'costume_badges_v01': '05-costume-badges-v0.1',
}
with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED) as z:
    for source, target in folders.items():
        directory = root / 'firmware/outputs' / source
        for p in sorted(directory.rglob('*')):
            if p.is_file() and p.suffix.lower() in {'.stl', '.3mf', '.blend'} and not any(label in p.stem.lower() for label in ('assembled', 'concept')):
                z.write(p, str(Path('firmware/outputs') / source / p.relative_to(directory)))
    for name in ['README.md', 'LICENSE', 'docs/PRINTING.md', 'docs/WIRING.md', 'docs/REPRODUCING.md', 'docs/SOURCES.md', 'docs/VALIDATION.md']:
        z.write(root / name, name)
    for p in sorted((root / 'docs/images').glob('*')):
        if p.is_file():
            z.write(p, str(p.relative_to(root)))
checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
(dest / 'SHA256SUMS.txt').write_text(f'{checksum}  {archive.name}\n')
print(f'{archive.name}: {archive.stat().st_size:,} bytes; SHA256 {checksum}')
