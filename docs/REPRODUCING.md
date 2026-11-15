# Reproducing the firmware, demo and CAD

The public source preserves v0.12.1 behavior. The certificate recipient has been
generalized to Radio Cadet. The device was last flashed during development on
27 October 2026; the public build does not change or reflash that device.

## Software

From `firmware/`, with PlatformIO Core, Python 3, Node.js and `clang++` installed:

```sh
pio run -e cores3 -e cores3_legacy_epaper
python3 tools/test_native.py
node test/preview_test.cjs
python3 tools/render_paper_preview.py
python3 tools/render_frequency_preview.py
python3 tools/build_preview.py
python3 -m http.server 8765 --bind 127.0.0.1 --directory preview
```

Render paper after the PlatformIO build: its native renderer uses the installed
Adafruit bitmap fonts under `.pio/libdeps/cores3`. Frequency and volume artwork
are drawn from the production C++ headers. `make_meter.py` regenerates the meter
texture and requires Pillow. Preview generation uses the included meter texture.

With Pillow installed, `tools/render_startup_preview.py` and
`tools/render_charging_preview.py` export the production LCD artwork as PNG/GIF.
These are artwork previews, not power-system emulation.

## CAD

The saved models and mesh generators use millimetres. The generators were built
with **Blender 5.2.2** and use its MANIFOLD Boolean solver; earlier Blender versions
may need adaptation. Run from `firmware/`, with Blender on PATH:

```sh
blender --background --factory-startup --python-exit-code 1 --python blender/build_bottom_v11.py
blender --background --factory-startup --python-exit-code 1 --python blender/build_front_v11.py
blender --background --factory-startup --python-exit-code 1 --python blender/build_tuning_knob_v02.py
blender --background --factory-startup --python-exit-code 1 --python blender/build_top_button_key_v01.py
blender --background --factory-startup --python-exit-code 1 --python blender/build_costume_badges_v01.py
```

Bottom v1.1 opens the included `outputs/usb_access_v02/` seed scene. Front v1.1
is generated from parameters; its historical assembly illustration imports the
included v0.8 bottom. That illustration is not a new combined v1.1 assembly test.
The current standalone bottom STL is v1.1. Do not substitute the v0.8 dependency.

Generators replace their output scenes. Save manual edits separately or edit
the source. Existing STL/3MF files are ready to inspect without installing Blender.
The saved 3MF files retain their A1 profiles and colour assignments; re-slice them
in Bambu Studio, or import the STL into your slicer. No automated print is sent.

## Release downloads

From the repository root:

```sh
python3 tools/package_release.py
```

This writes a current-parts print ZIP and SHA-256 checksum file under `dist/`.
It excludes older generator seeds, captured photos, local logs and machine caches.
