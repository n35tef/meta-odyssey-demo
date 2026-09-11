SUMMARY = "Odyssey dashboard demo (LVGL, ILI9486 framebuffer + XPT2046 touch)"
DESCRIPTION = "Kiosk HMI scaffold: an LVGL speedometer drawn straight to /dev/fb0 \
with touch via evdev. Replace build_dashboard() in main.c with the real UI."
HOMEPAGE = "https://lvgl.io/"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://lvgl/LICENCE.txt;md5=bf1198c89ae87f043108cea62460b03a"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# LVGL is vendored (pinned to the same rev meta-oe uses for 9.1.0)
SRC_URI = " \
    git://github.com/lvgl/lvgl;protocol=https;nobranch=1;destsuffix=git/lvgl \
    file://CMakeLists.txt \
    file://main.c \
    file://dashboard.service \
"
SRCREV = "e1c0b21b2723d391b885de4b2ee5cc997eccca91"

S = "${WORKDIR}/git"

inherit cmake systemd

SYSTEMD_SERVICE:${PN} = "dashboard.service"
SYSTEMD_AUTO_ENABLE = "enable"

RDEPENDS:${PN} += "kernel-module-fb-ili9486 kernel-module-fbtft kernel-module-ads7846"

# Drop our app sources next to the vendored lvgl tree and generate lv_conf.h
# from lvgl's template with just the values we need (mirrors meta-oe lv-conf.inc).
do_configure:prepend() {
    for d in "${UNPACKDIR}" "${WORKDIR}"; do
        if [ -f "$d/main.c" ]; then
            install -m0644 "$d/main.c" "${S}/main.c"
            install -m0644 "$d/CMakeLists.txt" "${S}/CMakeLists.txt"
            break
        fi
    done

    cp "${S}/lvgl/lv_conf_template.h" "${S}/lv_conf.h"
    sed -i \
        -e 's|#if 0 /\*Set it to "1" to enable content\*/|#if 1|' \
        -e 's|^\( *#define LV_COLOR_DEPTH \).*|\1 16|' \
        -e 's|^\( *#define LV_USE_LINUX_FBDEV \).*|\1 1|' \
        -e 's|^\( *#define LV_USE_EVDEV \).*|\1 1|' \
        -e 's|^\( *#define LV_USE_LOG \).*|\1 1|' \
        -e 's|^\( *#define LV_LOG_PRINTF \).*|\1 1|' \
        -e 's|^\( *#define LV_THEME_DEFAULT_DARK \).*|\1 1|' \
        -e 's|^\( *#define LV_FONT_MONTSERRAT_20 \).*|\1 1|' \
        -e 's|^\( *#define LV_FONT_MONTSERRAT_28 \).*|\1 1|' \
        -e 's|^\( *#define LV_FONT_MONTSERRAT_40 \).*|\1 1|' \
        "${S}/lv_conf.h"
}

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    for d in "${UNPACKDIR}" "${WORKDIR}"; do
        if [ -f "$d/dashboard.service" ]; then
            install -m0644 "$d/dashboard.service" ${D}${systemd_system_unitdir}/dashboard.service
            break
        fi
    done
}
