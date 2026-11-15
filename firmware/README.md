# CoreS3 firmware

Start with the [project README](../README.md), [wiring](../docs/WIRING.md),
[build instructions](../docs/REPRODUCING.md) and [validation](../docs/VALIDATION.md).

This is the public version of v0.12.1. The default certificate recipient is
Radio Cadet; personalize `include/PaperArtwork.h`. Normal firmware uploads retain
NVS volume settings and LittleFS photos. Never erase or upload a filesystem to a
device containing photos you want to keep.

The browser simulation covers the story, tuning, meter, volume and certificate;
camera, boot, power and ePaper hardware behavior require the physical radio.
