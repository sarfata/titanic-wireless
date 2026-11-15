# Printing and assembling the radio

All parts use millimetres; print at **100%**. The ready-made 3MF projects target
a Bambu A1, 0.4 mm nozzle and PLA. Re-slice for your printer/material; do not send
embedded machine-specific instructions to a different printer.

| Part | STL | 3MF | Orientation |
| --- | --- | --- | --- |
| Bottom v1.1 | [STL](../firmware/outputs/bottom_v11/stl/01_Titanic_bottom_v11.stl) | [3MF](../firmware/outputs/bottom_v11/slicer/Titanic_bottom_v11.3mf) | Back on bed, open side up |
| Front v1.1 | [STL](../firmware/outputs/front_v11/stl/01_Titanic_front_v11.stl) | [3MF](../firmware/outputs/front_v11/slicer/Titanic_front_v11.3mf) | Decorative face up |
| Tuning knob v0.2 | [STL](../firmware/outputs/tuning_knob_v02/Titanic_tuning_knob_6mm_pushfit_v02.stl) | [3MF](../firmware/outputs/tuning_knob_v02/slicer/Titanic_tuning_knob_v02.3mf) | Face down, shaft socket up |
| Top mushroom button | [STL](../firmware/outputs/top_button_key_v01/01_Top_mushroom_button_6mm.stl) | [3MF](../firmware/outputs/top_button_key_v01/slicer_button/Titanic_top_button_v01.3mf) | Broad face down, socket up |
| Optional lever key | [Four STLs](../firmware/outputs/top_button_key_v01) | [3MF](../firmware/outputs/top_button_key_v01/slicer_key/Titanic_top_key_v01.3mf) | Use the supplied plate |
| Costume badges | [Three STLs](../firmware/outputs/costume_badges_v01) | [3MF](../firmware/outputs/costume_badges_v01/slicer/Titanic_gold_costume_badges_v01.3mf) | Flat backs down |

Editable `.blend` files are in each part directory; generators are in
`firmware/blender`. Start with bottom **v1.1**. The older v0.8 STL and USB v0.2
scene included under `outputs` are generator dependencies, not alternate print
recommendations. The release print ZIP contains only the current parts.

## Main enclosure

- Bottom: **222 × 140 × 37 mm** including strap loops. 0.20 mm layers, 3 walls,
  15% infill, no supports. Earlier A1 slice: about **4 h 24 min / 157 g**.
- Front: **200 × 140 mm**, 3 mm base. Black first; gold decoration begins above
  3 mm. Raised screw surrounds reach 6.2 mm overall. Face up, no supports;
  earlier A1 slice about **1 h 48 min / 57 g**, including purge overhead.
- The front and back make a **40 mm body**; screw surrounds extend to 43.2 mm.
- Four **M3 × 8** screws into M3 heat-set inserts. The screws project 5 mm into
  **4.4 mm diameter × 6 mm deep** pockets. M3 × 12 is too long for this joint.
  H3, H4 or H5 inserts can fit the depth if their outside diameter suits your
  print. The 4.4 mm pilot was selected with a physical fit coupon.
- Front screw-head wells are 6.8 mm wide and 3.2 mm deep. Verify your screw heads.

## Fit before tightening

The ePaper PCB rests at **34.1 mm** from the back. The measured PCB-underside to
glass stack was **2.9 mm**, placing the glass at the 37 mm rim. The front seats
flush, with **zero nominal glass clearance**. Dry-fit and tighten only enough
to seat the panel; never use the screws to force the glass into alignment.

The ePaper supports are shifted **3.4 mm toward the bottom edge** to align the
visible image with the front window. The upper-right support has a 1 mm relief
beside the modeled USB plug. **Independent ePaper PCB screw retention remains
unfinished**: the narrowed support is not an insert boss. Plan non-stressing PCB
retention before carrying the unit; do not clamp the glass as a substitute.

The CoreS3/DIN has a 55 × 55 mm pocket, 4.2 mm rear seats and low 9 mm corner
guides. The closed lid retains the module vertically. Camera faces the bottom;
USB faces left; DIN Ports B/C face the top. Low guides clear the modeled ports,
but actual plugs and wire bends still require a physical check.

The right-side **13 × 10 mm** cable opening was sized for a USB-C housing
**35 × 11.5 × 8 mm**, with a **3.9 mm** cable. Thread the connector through before
closing. It is a pass-through, not a strain-relief clamp.

Top and front encoder mounting bores are **8 mm**. The two strap slots are
**4.4 × 29 mm**; narrow webbing threads through them. Test loaded attachment
strength before wearing the prop. A printed loop is not a certified load fitting.

## Knobs and optional key

The tuning knob uses a **6 × 16 mm** blind push-fit socket for the measured
6 mm ribbed shaft projecting 17 mm above the panel. No grub screw. Its 13 mm
nut relief is 5.5 mm deep; a nominal 1 mm gap permits pressing without rubbing.
Print 0.20 mm, 4 walls, 30% gyroid, no supports; about 24 minutes.

The simple top button is 26 × 19 mm, with the same 6 × 16 mm socket. Top shaft
projection was assumed from the matching encoder, not independently measured.
Confirm free click and spring return. About 23 minutes / 4 g.

The optional lever kit uses **three M3 × H3 inserts**, **one M3 × 12 pivot** and
**two M3 × 8 screws** for its stop and finger button. Its insert pockets are only
3 mm deep: **H5 is not suitable here**. Glue only the base after a successful
dry-fit; do not glue the cap to the shaft or the moving lever. Adjust the stop
to the real switch travel. The encoder's spring must return the lever freely.
This operates the firmware's button gestures, not manually keyed Morse.

The badges are two 80 × 35 mm shoulder boards and a 54 × 62 mm anchor medallion.
Print in gold, flat backs down. They are decorative costume designs, not a claim
of historically accurate rank insignia.
