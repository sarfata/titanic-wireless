# Browser demo

Serve this folder with `python3 -m http.server 8765 --bind 127.0.0.1` and open
<http://127.0.0.1:8765>. It also works embedded as an iframe on the project page.

- Sound is off until explicitly enabled.
- Top button starts/stops; tune to 500 after SOS, or use Demo at 500.
- Tuning briefly shows amber frequency digits, then the meter returns.
- Volume button represents holding the hardware button; adjust with the slider.
- The completed reply stays for ten seconds, then the certificate appears.

`radio-simulator.js` models state transitions. `morse-data.json` is exported from
the production C++ headers. Artwork atlases and the certificate are rendered
from production drawing code. `tools/build_preview.py` creates the self-contained
`index.html` and `titanic-radio.html` fragment. Rebuild after source changes.

This is a behavioral simulation, not an ESP32 binary emulator. It omits camera
capture, USB dock, boot animation and deep sleep. Real ePaper refresh timing,
ghosting, input debounce and audio hardware require device tests. Hidden tabs
pause the simulation and mute audio. The generic certificate name is Radio Cadet.
