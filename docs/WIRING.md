# CoreS3 wiring

For **CoreS3 K128 + DIN base**, a monochrome Waveshare module with driver PCB,
and bare dry-contact EC11 encoders. GPIO numbers below are not physical header
pin numbers. Disconnect USB, external DC and the DIN battery before soldering.

| ePaper label | CoreS3 GPIO / supply | Connection |
| --- | --- | --- |
| VCC | 3V3 | M-Bus 3.3 V supply |
| GND | GND | Common ground |
| DIN | 17 | DIN Port C yellow |
| CLK | 18 | DIN Port C white |
| CS | 5 | M-Bus GPIO5 |
| DC | 6 | M-Bus GPIO6 |
| RST | 7 | M-Bus GPIO7; **not** RST/EN |
| BUSY | 8 | DIN Port B white |

Match labels and check continuity; display cable colours vary. Use 4-wire SPI
on the display module (BS = 0 on the photographed module). A raw ePaper panel
without the driver board cannot be connected using this table.

| Control | Signal | Connection |
| --- | --- | --- |
| Front EC11 A | GPIO2 | CoreS3 Port A yellow |
| Front EC11 B | GPIO1 | CoreS3 Port A white |
| Front EC11 common | GND | Port A black |
| Top EC11 push pair | GPIO9 and GND | DIN Port B yellow and black |

Leave all Grove **red 5 V wires disconnected and insulated**. Bare encoders need
no power wire. Port B white is already ePaper BUSY; do not use it for the key.
Leave the top encoder's three rotary contacts unused. Leave the front push pair
unused, or connect it in parallel with the top switch for identical behavior.

A typical EC11 has three rotary terminals and two switch terminals, but identify
your particular part with its drawing and a continuity meter. The switch pair
closes only while pressed. The rotary common alternately connects to A and B.
The metal mounting tabs are not signal pins.

For a reversed tuning direction, change `ENCODER_DIRECTION` in
`firmware/include/RadioConfig.h`. EC11 detent variants may need
`ENCODER_EDGES_PER_STEP = 4` instead of 2. Serial `encoder reverse` and
`encoder steps 4` allow temporary testing; these overrides reset at reboot.

The DIN prototype area is not a ready-made signal bus. Confirm each solder hole
against the actual header with continuity; do not infer net names from position
or from the older Core/Core2 numbering. Keep the display-end connector removable
and add strain relief so opening the case cannot pull on its pads.

References: [CoreS3](https://docs.m5stack.com/en/core/CoreS3),
[DIN base](https://docs.m5stack.com/en/base/Base_DIN),
[Waveshare 4.2-inch module](https://www.waveshare.com/wiki/4.2inch_e-Paper_Module).
