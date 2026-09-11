/*
 * Odyssey dashboard demo — LVGL on the ILI9486 SPI panel (/dev/fb0)
 * with XPT2046 touch (/dev/input/eventN).
 *
 * Scaffold: a dark 0..100 gauge with an animated sweep plus a live touch
 * read-out. Replace build_dashboard() with the real UI.
 *
 * Env overrides (set them in dashboard.service — no rebuild needed):
 *   LV_VIDEO_CARD   framebuffer node        (default: /dev/fb0)
 *   LV_TOUCH_DEV    touch input event node  (default: autodetect by name, else ABS_X, else event1)
 *   LV_TOUCH_CAL    "min_x min_y max_x max_y" raw-ADS7846 -> screen calibration
 *                   (default: 200 200 3900 3900; swap a pair to invert that axis)
 *   LV_TOUCH_SWAP   1 = swap X/Y (if touch feels transposed)
 *   LV_TOUCH_RAW    1 = skip calibration, show raw coords (to find LV_TOUCH_CAL)
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "lvgl/lvgl.h"

/* ---- tick source ---------------------------------------------------------- */
static uint32_t tick_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

/* ---- touch device autodetect --------------------------------------------- */
static int dev_has_abs_xy(int fd)
{
    unsigned long absbits[(ABS_MAX / (8 * sizeof(long))) + 1];
    memset(absbits, 0, sizeof(absbits));
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits) < 0) return 0;
    return (absbits[ABS_X / (8 * sizeof(long))] >> (ABS_X % (8 * sizeof(long)))) & 1;
}

/* Prefer a device whose name looks like a touchscreen; fall back to the first
 * one that reports ABS_X; last resort /dev/input/event1. Logs what it saw. */
static const char *find_touch_dev(void)
{
    const char *env = getenv("LV_TOUCH_DEV");
    if (env && *env) return env;

    static char best_abs[64] = "";
    static char best_named[64] = "";

    for (int i = 0; i < 32; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        char name[128] = "";
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        int abs_xy = dev_has_abs_xy(fd);
        close(fd);

        LV_LOG_USER("dashboard: %s  name=\"%s\"  abs_xy=%d", path, name, abs_xy);

        if (!abs_xy) continue;
        if (!best_abs[0]) snprintf(best_abs, sizeof(best_abs), "%s", path);

        char low[128];
        size_t n = 0;
        for (; name[n] && n < sizeof(low) - 1; n++) low[n] = (char)tolower((unsigned char)name[n]);
        low[n] = '\0';
        if (!best_named[0] && (strstr(low, "touch") || strstr(low, "ads7846") ||
                               strstr(low, "ts") || strstr(low, "xpt")))
            snprintf(best_named, sizeof(best_named), "%s", path);
    }

    if (best_named[0]) return best_named;
    if (best_abs[0])   return best_abs;
    return "/dev/input/event1";
}

/* ---- touch calibration -------------------------------------------------- */
static int g_touch_raw;               /* 1 = report raw coords */
static lv_indev_t *g_touch;           /* the evdev pointer indev, or NULL */

static void touch_setup(lv_indev_t *indev)
{
    if (!indev) {
        LV_LOG_ERROR("dashboard: lv_evdev_create FAILED (%s)", strerror(errno));
        return;
    }

    const char *sw = getenv("LV_TOUCH_SWAP");
    if (sw && atoi(sw)) {
        lv_evdev_set_swap_axes(indev, true);
        LV_LOG_USER("dashboard: touch X/Y swapped (LV_TOUCH_SWAP)");
    }

    const char *raw = getenv("LV_TOUCH_RAW");
    if (raw && atoi(raw)) {
        g_touch_raw = 1;
        LV_LOG_USER("dashboard: LV_TOUCH_RAW set — calibration off");
        return;
    }

    int min_x = 200, min_y = 200, max_x = 3900, max_y = 3900;
    const char *cal = getenv("LV_TOUCH_CAL");
    if (cal && sscanf(cal, "%d %d %d %d", &min_x, &min_y, &max_x, &max_y) == 4)
        LV_LOG_USER("dashboard: LV_TOUCH_CAL = %d %d %d %d", min_x, min_y, max_x, max_y);
    else
        LV_LOG_USER("dashboard: touch cal = default %d %d %d %d", min_x, min_y, max_x, max_y);

    lv_evdev_set_calibration(indev, min_x, min_y, max_x, max_y);
}

/* ---- dashboard UI ------------------------------------------------------- */
#define SPD_MAX 100

static lv_obj_t *arc_speed;
static lv_obj_t *lbl_speed;
static lv_obj_t *lbl_touch;

/* Poll the input device every tick instead of relying on event bubbling to the
 * screen (a plain screen object is not LV_OBJ_FLAG_CLICKABLE by default, so
 * LV_EVENT_PRESSING never fires on it). */
static void poll_touch(lv_timer_t *t)
{
    LV_UNUSED(t);
    if (!g_touch) {
        lv_label_set_text(lbl_touch, "touch: NO DEVICE");
        return;
    }
    if (lv_indev_get_state(g_touch) == LV_INDEV_STATE_PRESSED) {
        lv_point_t p;
        lv_indev_get_point(g_touch, &p);
        lv_label_set_text_fmt(lbl_touch, g_touch_raw ? "raw  %d, %d" : "touch  %d, %d",
                              (int)p.x, (int)p.y);
    }
}

static void anim_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    static float phase = 0.0f;
    phase += 0.05f;
    int spd = (int)((SPD_MAX / 2.0f) + (SPD_MAX / 2.0f) * sinf(phase));  /* 0..SPD_MAX */
    lv_arc_set_value(arc_speed, spd);
    lv_label_set_text_fmt(lbl_speed, "%d", spd);
}

static void build_dashboard(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a10), 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    arc_speed = lv_arc_create(scr);
    lv_obj_set_size(arc_speed, 280, 280);
    lv_obj_center(arc_speed);
    lv_arc_set_rotation(arc_speed, 135);
    lv_arc_set_bg_angles(arc_speed, 0, 270);
    lv_arc_set_range(arc_speed, 0, SPD_MAX);
    lv_arc_set_value(arc_speed, 0);
    lv_obj_remove_style(arc_speed, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc_speed, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_speed, 14, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_speed, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_speed, lv_color_hex(0x1c2333), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_speed, lv_color_hex(0x36c2ff), LV_PART_INDICATOR);

    /* brand label — inside the dial, sitting just above the number */
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "ODYSSEY");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x7c8db5), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -46);

    lbl_speed = lv_label_create(scr);
    lv_label_set_text(lbl_speed, "0");
    lv_obj_set_style_text_color(lbl_speed, lv_color_hex(0xf0f4ff), 0);
    lv_obj_set_style_text_font(lbl_speed, &lv_font_montserrat_40, 0);
    lv_obj_align(lbl_speed, LV_ALIGN_CENTER, 0, 6);

    lbl_touch = lv_label_create(scr);
    lv_label_set_text(lbl_touch, "touch  -, -");
    lv_obj_set_style_text_color(lbl_touch, lv_color_hex(0x4a5878), 0);
    lv_obj_align(lbl_touch, LV_ALIGN_BOTTOM_MID, 0, -6);

    lv_timer_create(poll_touch, 50, NULL);
    lv_timer_create(anim_cb, 40, NULL);
}

/* ---- main ------------------------------------------------------------- */
int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);   /* so LV_LOG_* reaches the journal */

    lv_init();
    lv_tick_set_cb(tick_ms);

    const char *fb = getenv("LV_VIDEO_CARD");
    lv_display_t *disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, fb && *fb ? fb : "/dev/fb0");

    const char *tdev = find_touch_dev();
    LV_LOG_USER("dashboard: using touch device = %s", tdev);
    g_touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, tdev);
    touch_setup(g_touch);

    build_dashboard();

    for (;;) {
        lv_timer_handler();
        usleep(5000);
    }
    return 0;
}
