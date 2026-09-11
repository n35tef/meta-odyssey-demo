# meta-odyssey-demo

Optional add-on layer for the Seeed Studio Odyssey-STM32MP157C. Adds:

- The **Waveshare 3.5" RPi LCD** (ILI9486 480×320 SPI panel + XPT2046
  resistive touch) wired onto the 40-pin header (SPI5) - see
  [`docs/spi-display.md`](docs/spi-display.md) for the pin map and bring-up
  commands.
- Two kernel patches that make the XPT2046 on this panel actually report
  touch events (see their commit messages for the root cause). **Do not drop
  either patch on a kernel bump without re-testing touch on real hardware** -
  the second one fixes a quirk specific to this board's XPT2046 die that the
  upstream fix alone doesn't cover.
- `dashboard`: a minimal LVGL v9 kiosk app (dark speedometer + live touch
  read-out) rendering straight to `/dev/fb0` - a starting point for a real
  HMI, not a finished product.
- `odyssey-dashboard`: a minimal image (`st-image-core` + the app, no Weston)
  that boots straight into the kiosk.

## Requires

The [meta-st-odyssey](../meta-st-odyssey) base layer, already in
`bblayers.conf`, for the board to boot at all. This layer only adds the HAT
and the demo app on top of a board that already works.

## Add the layer

```bash
bitbake-layers add-layer ../meta-odyssey-demo
```

## Build

```bash
bitbake odyssey-dashboard      # kiosk image, boots straight to the LVGL app
# or, to add the panel + touch to the full desktop image instead:
bitbake st-image-weston        # then uncomment the tool list in
                                # recipes-st/images/st-image-weston.bbappend
                                # if you also want fbset/evtest/spitools etc.
```

Flash the same way as the base layer's README describes.

## What's in here

| Path | Purpose |
|---|---|
| `recipes-kernel/linux/` | SPI5 panel/touch DT + the two ads7846 fixes + the fbtft kconfig fragment |
| `recipes-hmi/dashboard/` | The LVGL kiosk app |
| `recipes-st/images/odyssey-dashboard.bb` | The kiosk image |
| `recipes-st/images/st-image-weston.bbappend` | Optional bring-up tools for a desktop-image build |
| `docs/spi-display.md` | Pin map, wiring, bring-up commands |

## Hardware notes

- The panel is on `/dev/fb0` - the OP-TEE DT this board uses has LTDC/DSI
  disabled, so there is no `/dev/dri` node and no GPU (Vivante GC Nano)
  acceleration on this board. LVGL's software renderer is the only rendering
  path; SPI bandwidth (~30 fps full-screen at most) is the real ceiling.
- Touch calibration: raw range 0..4095. `dashboard.service` has commented
  `Environment=LV_TOUCH_CAL=...` / `LV_TOUCH_SWAP=1` / `LV_TOUCH_RAW=1` lines
  for tuning at runtime with no rebuild - see the service file.
