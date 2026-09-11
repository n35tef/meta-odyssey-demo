# SPI display on the Odyssey 40-pin header

Adds the **Waveshare 3.5" RPi LCD (C)** — ILI9486 480×320 SPI panel + XPT2046
resistive touch — on the Odyssey-STM32MP157C 40-pin GPIO header, driven by the
kernel `fbtft` (`fb_ili9486`) driver.

## Wiring (plug the HAT straight onto the 40-pin header)

| HAT signal | Header pin | STM32 pin | Notes |
|---|---|---|---|
| SPI SCLK  | 23 | PH6 | SPI5_SCK (AF5) |
| SPI MOSI  | 19 | PF9 | SPI5_MOSI (AF5) |
| SPI MISO  | 21 | PH7 | SPI5_MISO (AF5) |
| LCD CS    | 24 | PF6 | GPIO chip-select (SPI5 CS0) |
| Touch CS  | 26 | PF3 | GPIO chip-select (SPI5 CS1) |
| LCD DC/RS | 18 | PE7 | |
| LCD RESET | 22 | PE8 | active low |
| Touch IRQ | 11 | PD4 | falling edge |
| Backlight | —  | —   | hard-wired on, not SW-controllable on this panel |

SPI clock is capped at **32 MHz** (STM32MP157 SPI5 can't reach the RPi overlay's
115 MHz). Expect ~16–20 fps for a full-screen redraw; partial updates are fine.

## What lands in the image

- DT: `&spi5` gains `display@0` (`ilitek,ili9486`) and `touchscreen@1`
  (`ti,ads7846`) — see the SPI5 patch in `recipes-kernel/linux/`.
- Kernel config fragment `fragment-90-spi-tft-display.config` turns
  `CONFIG_STAGING` back on and builds `fb_ili9486`/`fbtft`/`ads7846` as
  modules (autoload from the DT compatibles, no modprobe needed).
- LTDC/DSI are disabled on this board, so the ILI9486 panel is the **only**
  framebuffer: **`/dev/fb0`** (480×320, RGB565), touch as an
  `/dev/input/eventN` (`ADS7846 Touchscreen`).
- Bring-up tools in the rootfs: `fbset` (+`con2fbmap`), `evtest`,
  `libgpiod-tools`, `fbgrab`.

## Bring-up checks on the board

```sh
dmesg | grep -Ei 'fbtft|ili9486|ads7846|spi5|44009000'
ls -l /dev/fb*                       # expect fb0 (only framebuffer on this board)
fbset -fb /dev/fb0                    # expect 480x320, 16bpp
cat /sys/class/graphics/fb0/name     # -> "ili9486"

# paint the panel / clear it
dd if=/dev/urandom of=/dev/fb0 bs=$((480*320*2)) count=1
cat /dev/zero > /dev/fb0             # black

# touch
evtest /dev/input/by-path/*ads7846* 2>/dev/null || evtest   # pick the ADS7846 device
```

## Using it

- **Linux console on the panel**: it is `/dev/fb0` so `fbcon` maps to it by
  default; if you re-enabled LTDC/DSI use `fbcon=map:1` / `con2fbmap 1 1`.
- **A framebuffer app** (LVGL, Qt `-platform linuxfb:fb=/dev/fb0`, DirectFB,
  SDL `SDL_FBDEV=/dev/fb0`): point it at `/dev/fb0`.
- Weston/Wayland can only use its software (pixman) backend on this panel —
  fbtft exposes no DRM device, so there is no GPU/DRM acceleration on this
  board. See the `odyssey-dashboard` kiosk image for the no-compositor path.

## Calibrating touch

`ti,swap-xy` is set to match the `rotate=90` landscape orientation; if X/Y are
mirrored, adjust `ti,swap-xy` / add `touchscreen-inverted-x|y` in the DT patch,
or calibrate at runtime with `libinput` / `weston-touch-calibrator` /
`xinput_calibrator` for your UI stack.
