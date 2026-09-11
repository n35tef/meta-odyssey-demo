/*
 * Overlay applied on top of lvgl's lv_conf_template.h at do_configure time
 * (see dashboard_1.0.bb). Only the values we care about are sed'd in;
 * this file is here purely as documentation of the intended config:
 *
 *   LV_COLOR_DEPTH            16     ILI9486 fbtft framebuffer is RGB565
 *   LV_USE_LINUX_FBDEV        1      draw to /dev/fb0
 *   LV_USE_EVDEV             1      XPT2046 touch via /dev/input/eventN
 *   LV_USE_LOG               1
 *   LV_LOG_PRINTF            1
 *   LV_THEME_DEFAULT_DARK    1
 *   LV_FONT_MONTSERRAT_20/28/40   enabled for the gauge readout
 *
 * If on-target colours look inverted, flip LV_COLOR_16_SWAP (1) or the
 * fbtft "bgr" property in the kernel DT.
 */
