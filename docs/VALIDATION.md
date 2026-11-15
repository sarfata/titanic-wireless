# Validation record

Public package prepared **14 November 2026**, based on development firmware
v0.12.1. The public certificate says Radio Cadet. No device was reflashed as part
of publication, and no private photographs or serial logs are distributed.

## Software checked for this publication

- PlatformIO builds passed for `cores3` and `cores3_legacy_epaper`, with the pinned
  dependencies in `firmware/platformio.ini`.
- All eleven native C++ test suites passed: radio logic, needle motion, audio,
  letter reveal, quadrature, tuning view, button/volume controls, power, wake,
  charging policy and startup sequencing.
- JavaScript regression checks passed for the exchange, tuning lock, cancellation,
  letter reveal, certificate/reset, frequency view and volume behavior.
- Production ePaper artwork and the standalone browser demo were regenerated.
  The public front-panel render was regenerated with the generic certificate;
  the exported front STL's SHA-256 remained identical to the validated source.
- Website integration checked at desktop and phone width, including the embedded
  demo, responsive layout and local images. This does not test physical audio
  quality, ePaper ghosting or timing on a real display.

The optional `docs/github-actions-check.yml` template repeats firmware builds,
native/JavaScript checks and preview generation. It is not enabled: the publishing
login lacks GitHub workflow permission. To enable CI later, copy it to
`.github/workflows/check.yml` using an account with that permission.

## Earlier development evidence

The original firmware was flashed to a CoreS3 and checked during September–October
2026. User feedback confirmed working controls, audio/needle animation, ePaper,
camera photos and battery operation. Serial/software checks covered startup and
timed deep-sleep recovery. These observations are not a fresh hardware test of
this public checkout or proof of full-day battery life.

The user physically checked the LCD opening, ePaper frame, insert pilot coupon,
first shell and 2.9 mm ePaper stack. The current bottom v1.1 includes later port
and USB clearance changes. Geometry/slicing reports shipped with the parts are
earlier development checks, not measurements of every finished print.

## Still needs a physical check

- Whole-board sleep current and runtime from a fully charged battery.
- Actual connector bends and clearances in the revised bottom.
- Independent, non-stressing ePaper PCB retention; the narrowed NE support is not
  an insert boss. See [Printing](PRINTING.md).
- Optional key spring return, top shaft projection and insert fit.
- Camera field of view through the assembled front.

The public write-up labels historical renders and the early assembly animation
as development illustrations. Final-product photographs will be added separately.
