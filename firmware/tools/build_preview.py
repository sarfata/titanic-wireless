"""Build a dependency-free browser/inline preview from the firmware's Morse data."""
import base64
import json
import html as html_codec
import re
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
output = root / 'outputs'
output.mkdir(exist_ok=True)
subprocess.run(['clang++', '-std=c++17', '-Wall', '-Wextra', '-pedantic', '-I', str(root / 'include'),
                str(root / 'tools/preview_data.cpp'), '-o', str(output / 'preview_data')], check=True)
data = subprocess.check_output([str(output / 'preview_data')], text=True)
json.loads(data)
(root / 'preview/morse-data.json').write_text(data)
html = (root / 'preview/preview-template.html').read_text()
html = html.replace('/*MORSE_DATA*/', data.strip())
html = html.replace('/*SIMULATOR_SOURCE*/', (root / 'preview/radio-simulator.js').read_text())
html = html.replace('/*METER_FACE*/', base64.b64encode((root / 'outputs/meter_face.png').read_bytes()).decode())
html = html.replace('/*FREQUENCY_ATLAS*/', base64.b64encode((root / 'outputs/frequency_atlas_v07.png').read_bytes()).decode())
html = html.replace('/*VOLUME_ATLAS*/', base64.b64encode((root / 'outputs/volume_atlas_v08.png').read_bytes()).decode())
html = html.replace('/*FONT_DATA*/', base64.b64encode((root / 'licenses/PatrickHand-Regular.ttf').read_bytes()).decode())
html = html.replace('/*CERTIFICATE_IMAGE*/', base64.b64encode((root / 'outputs/epaper_certificate_v06.png').read_bytes()).decode())
(root / 'preview/titanic-radio.html').write_text(html)
print('Built preview/titanic-radio.html from RadioLogic.h and RadioConfig.h')

# A standalone document can be served anywhere, including an iframe on the project page.
index = root / 'preview/index.html'
index.write_text('<!doctype html>\n<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="color-scheme" content="light"><title>Titanic wireless — interactive demo</title><style>html{background:#fff;color:#262626}body{margin:0;padding:12px;max-width:900px;margin-inline:auto}a{color:#4a3f8a}p{font:14px/1.6 system-ui}</style></head><body>' + html + '<p>Behavioral browser demo. Camera, boot animation, charging and deep sleep run on the physical radio. Sound starts only when enabled.</p></body></html>\n')
print('Built standalone preview/index.html')
