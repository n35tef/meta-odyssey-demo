SUMMARY = "Odyssey LVGL dashboard kiosk image (no Weston)"
DESCRIPTION = "Minimal OpenSTLinux image that boots straight to the dashboard \
LVGL app on the ILI9486 SPI panel. Same firmware stack as st-image-weston \
(TF-A + OP-TEE + U-Boot), just a tiny rootfs with one app."

require recipes-st/images/st-image-core.bb

# the app plus the kernel modules the SPI panel / touch need
IMAGE_INSTALL:append = " \
    dashboard \
    kernel-modules \
"

# evtest reads the raw ADS7846 range for LV_TOUCH_CAL (see docs/spi-display.md)
IMAGE_INSTALL:append = " evtest"

# onboard WiFi/BT (AP6236) firmware and userspace
IMAGE_INSTALL:append = " \
    linux-firmware-addons-bcm43xx \
    iw \
    wpa-supplicant \
    bluez5 \
"

# bring-up tools for the panel, touch controller and SPI bus - not installed by default
# IMAGE_INSTALL:append = " fbset fbgrab libgpiod-tools spitools spidev-test"

# no package feed / no Weston desktop here
IMAGE_FEATURES:remove = "package-management"

# boot straight to the app (multi-user, no graphical.target / display-manager)
SYSTEMD_DEFAULT_TARGET = "multi-user.target"

# shrink the sdcard flash layout (rootfs slot 1.75 GiB); anonymous python because the
# flashlayout class re-expands this key after normal assignments
python () {
    d.setVar('FLASHLAYOUT_PARTITION_SIZE:sdcard:rootfs', '1835008')
}
