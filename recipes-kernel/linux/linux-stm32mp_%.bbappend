FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Waveshare 3.5" RPi LCD (ILI9486 + XPT2046 touch) on the Odyssey 40-pin header (SPI5).
# 0001  panel + touch controller on SPI5 (pinmux, cs-gpios, ili9486/ads7846 nodes)
# 0002  ads7846: backport of 781a07da9bb9 + fixes (XPT2046 command-register lock-up)
# 0003  ads7846: probe bootstrap ends on an X conversion, or this XPT2046 never arms nPENIRQ
#       (board-specific; re-test touch on hardware before dropping on a kernel bump)
SRC_URI += " \
    file://${LINUX_VERSION}/${LINUX_VERSION}${LINUX_SUBVERSION}/0001-ARM-dts-stm32mp157c-odyssey-add-SPI5-ILI9486-display.patch \
    file://${LINUX_VERSION}/${LINUX_VERSION}${LINUX_SUBVERSION}/0002-Input-ads7846-add-dummy-command-register-clearing-cyc.patch \
    file://${LINUX_VERSION}/${LINUX_VERSION}${LINUX_SUBVERSION}/0003-Input-ads7846-arm-nPENIRQ-with-an-X-position-bootstr.patch \
    file://odyssey/fragment-90-spi-tft-display.config;subdir=fragments \
"

# fbtft lives in staging, which ST's cleanup fragment turns off
KERNEL_CONFIG_FRAGMENTS:append = " ${WORKDIR}/fragments/odyssey/fragment-90-spi-tft-display.config"
