// testctl.c — reads lines from the USB CDC driver on its own task and injects events into
// the app under the LVGL lock, so the app's real handlers run exactly as they do for hardware input.
#include "testctl.h"
#include <limits.h>
#include "app.h"
#include "keyboard.h"
#include "ui.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "soc/rtc_cntl_reg.h"
#include "tusb_cdc_acm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "mbedtls/base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "testctl";

#define CMD_LINE_MAX   64
#define LOCK_MS    500

static void reply_state(void)
{
    char line[96];
    app_state_line(line, sizeof(line));
    printf("> %s\n", line);
    fflush(stdout);
}

// Snapshot the active screen into a PSRAM buffer and stream it as base64 rows.
static void do_snap(void)
{
    static uint8_t *s_pixels;
    const lv_color_format_t cf = LV_COLOR_FORMAT_RGB565;
    lv_obj_t *scr = lv_screen_active();
    uint32_t w = lv_obj_get_width(scr), h = lv_obj_get_height(scr);
    uint32_t stride = lv_draw_buf_width_to_stride(w, cf);
    size_t size = (size_t)stride * h;

    if (!s_pixels) s_pixels = heap_caps_aligned_alloc(64, size, MALLOC_CAP_SPIRAM);
    if (!s_pixels) { printf("> error no memory for snapshot\n"); fflush(stdout); return; }

    lv_draw_buf_t buf;
    lv_draw_buf_init(&buf, w, h, cf, stride, s_pixels, size);
    lv_draw_buf_set_flag(&buf, LV_IMAGE_FLAGS_MODIFIABLE);

    if (!ui_lock(LOCK_MS)) { printf("> error lvgl lock\n"); fflush(stdout); return; }
    lv_result_t r = lv_snapshot_take_to_draw_buf(scr, cf, &buf);
    ui_unlock();
    if (r != LV_RESULT_OK) { printf("> error snapshot failed\n"); fflush(stdout); return; }

    // One base64 row per line, each written in ONE call so a log line from another task cannot
    // land inside it; logging is muted for the duration of the dump for the same reason.
    size_t b64_cap = ((stride + 2) / 3) * 4 + 1;
    unsigned char *row = malloc(2 + b64_cap + 2);
    if (!row) { printf("> error no memory for base64\n"); fflush(stdout); return; }
    esp_log_level_t saved = esp_log_level_get("*");
    esp_log_level_set("*", ESP_LOG_NONE);
    printf("SNAP BEGIN %u %u RGB565\n", (unsigned)w, (unsigned)h);
    fflush(stdout);
    for (uint32_t y = 0; y < h; y++) {
        size_t olen = 0;
        row[0] = 'S'; row[1] = ':';
        mbedtls_base64_encode(row + 2, b64_cap, &olen, s_pixels + (size_t)y * stride, w * 2);
        row[2 + olen] = '\n';
        fwrite(row, 1, 3 + olen, stdout);
        fflush(stdout);
        if (tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, pdMS_TO_TICKS(1000)) != ESP_OK) {
            esp_log_level_set("*", saved);
            free(row);
            return;
        }
    }
    printf("SNAP END\n");
    fflush(stdout);
    esp_log_level_set("*", saved);
    free(row);
}

static void handle(char *line)
{
    while (*line == ' ') line++;
    size_t n = strlen(line);
    while (n && (line[n - 1] == '\r' || line[n - 1] == ' ')) line[--n] = '\0';
    if (!n) return;

    // Software recovery path now that the app owns USB OTG instead of Serial/JTAG.
    if (!strcmp(line, "bootloader")) {
        REG_SET_BIT(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
        esp_restart();
    }

    if (!strcmp(line, "ping")) { printf("> pong\n"); fflush(stdout); return; }
    if (!strcmp(line, "jiggle")) {
        if (!ui_lock(LOCK_MS)) { printf("> error lvgl lock\n"); fflush(stdout); return; }
        bool allowed = app_is_unlocked();
        bool queued = allowed && keyboard_test_jiggle();
        ui_unlock();
        printf("> %s\n", !allowed ? "error locked" : queued ? "mouse queued" : "error mouse queue full");
        fflush(stdout);
        return;
    }
    if (!strcmp(line, "diag")) {
        printf("> reset_reason=%d stack_free=%u\n", (int)esp_reset_reason(),
               (unsigned)uxTaskGetStackHighWaterMark(NULL));
        fflush(stdout);
        return;
    }
    if (!strcmp(line, "snap")) { do_snap(); return; }
    if (!strcmp(line, "state")) { reply_state(); return; }

    int button = 0;
    if (!strncmp(line, "button ", 7)) {
        char *end;
        long id = strtol(line + 7, &end, 10);
        if (*end == 0 && id >= 1 && id <= INT_MAX) button = (int)id;
    }
    bool vertical = !strcmp(line, "swipe up") || !strcmp(line, "swipe down");
    bool swipe = !strcmp(line, "swipe left") || !strcmp(line, "swipe right");
    if (!strcmp(line, "tap") || !strcmp(line, "hold") || !strcmp(line, "reset") ||
        !strncmp(line, "knob ", 5) || button || swipe || vertical) {
        if (!ui_lock(LOCK_MS)) { printf("> error lvgl lock\n"); fflush(stdout); return; }
        if (button)              app_inject_button(button);
        else if (vertical)       app_inject_vertical_swipe(!strcmp(line, "swipe up"));
        else if (swipe)          app_inject_swipe(!strcmp(line, "swipe left"));
        else if (line[0] == 't') app_inject_tap();
        else if (line[0] == 'h') app_inject_hold();
        else if (line[0] == 'r') app_inject_reset();
        else                     app_inject_knob((int)strtol(line + 5, NULL, 10));
        ui_unlock();
        reply_state();
        return;
    }
    printf("> error unknown command '%s'\n", line);
    fflush(stdout);
}

static void testctl_task(void *arg)
{
    (void)arg;
    char line[CMD_LINE_MAX];
    size_t len = 0;
    uint8_t buf[32];
    for (;;) {
        size_t n = 0;
        ESP_ERROR_CHECK(tinyusb_cdcacm_read(TINYUSB_CDC_ACM_0, buf, sizeof(buf), &n));
        for (size_t i = 0; i < n; i++) {
            char c = (char)buf[i];
            if (c == '\n') { line[len] = '\0'; handle(line); len = 0; }
            else if (len < CMD_LINE_MAX - 1) line[len++] = c;
            else len = 0;   // overlong line: drop it
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void testctl_init(void)
{
    // Snapshotting button widgets adds a deeper LVGL draw call chain than plain labels.
    xTaskCreate(testctl_task, "testctl", 8192, NULL, 3, NULL);
    ESP_LOGI(TAG, "serial test console up: knob <±N> | tap | hold | reset | state | snap | ping");
}
