# Sources and credits

- Thomas Sarlandie: project, assembly and physical fit feedback; developed with
  Codex through iterative firmware, CAD and bench checks in September–October 2026.
- [M5Stack CoreS3 documentation](https://docs.m5stack.com/en/core/CoreS3) and
  [DIN base](https://docs.m5stack.com/en/base/Base_DIN): pin assignments and hardware.
- [Waveshare 4.2-inch ePaper module](https://www.waveshare.com/wiki/4.2inch_e-Paper_Module).
- Firmware dependencies are fetched by PlatformIO: M5Unified, M5GFX, GxEPD2,
  Adafruit GFX and Adafruit BusIO. Their upstream licenses apply to those libraries.
- Patrick Hand by Patrick Wagesreiter: SIL Open Font License, bundled in
  `firmware/licenses`. Its generated bitmap header is used on the display.
- Blender renders, model screenshots and slicer previews document this project.
  Rendered electronics are reference representations, not fabrication drawings.

## Historic inspiration

The nominal 500 kHz display corresponds to 600 metres. Titanic used this shared
ship wavelength for ordinary traffic as well as distress; do not describe it as
an exclusively reserved distress channel in April 1912.

- [US inquiry: Harold Cottam](https://www.titanicinquiry.org/USInq/AmInq02Cottam02.php)
- [US inquiry: Cyril Evans](https://www.titanicinquiry.org/USInq/AmInq08EvansCF02.php)
- [ITU history of early maritime radio](https://www.itu.int/itunews/manager/display.asp?ext=html&ipage=pioneers&issue=06&lang=en&year=2006)

The short messages **SOS WE NEED HELP** and **WE ARE COMING** are deliberately
simplified game dialogue. The box and badges are costume designs, not replicas
of Titanic equipment or authenticated insignia.
