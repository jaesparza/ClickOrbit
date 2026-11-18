// Touch-only controls. Actual USB reports run on the keyboard worker task.
#include "app.h"
#include "app_screen.h"
#include "pomodoro_view.h"
#include "config_view.h"
#include "keyboard.h"
#include "haptic.h"
#include "ui.h"
#include "knob.h"
#include "lock_combination.h"
#include "backlight.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>

static int s_last_string;
static bool s_unlocked;
static lock_combination_t s_combination;
#define IDLE_DIM_US (5LL * 60 * 1000000)
#define DIM_LEVEL_PCT 20
static int64_t s_last_activity_us;
static bool s_dimmed;

static void note_activity(void)
{
    s_last_activity_us = esp_timer_get_time();
    if (s_dimmed) {
        backlight_set_percent(80);
        s_dimmed = false;
    }
}

static void idle_dim_tick(lv_timer_t *timer)
{
    (void)timer;
    if (!s_dimmed && esp_timer_get_time() - s_last_activity_us >= IDLE_DIM_US) {
        backlight_set_percent(DIM_LEVEL_PCT);
        s_dimmed = true;
    }
}

static void on_screen_activity(lv_event_t *event)
{
    (void)event;
    note_activity();
}
bool app_is_unlocked(void) { return s_unlocked; }
static void lock_device(void)
{
    s_unlocked = false;
    s_combination = (lock_combination_t){0};
    keyboard_set_jiggler(false);
    app_screen_set_jiggler(false);
    app_screen_set_locked(true);
}
static void on_button(lv_event_t *e)
{
    note_activity();
    if (!s_unlocked) return;
    int number = (int)(intptr_t)lv_event_get_user_data(e);
    if (pomo_view_visible()) {
        if (number == POMO_TOGGLE) pomo_view_toggle();
        else if (number == POMO_MENU) app_screen_show_pomodoro(false);
        else if (number == POMO_RESET) pomo_view_restart();
        return;
    }
    if (number == POMO_OPEN) {
        app_screen_show_pomodoro(true);
    } else if (number == 4) {
        lock_device();
    } else if (number == 2) {
        keyboard_set_jiggler(!keyboard_jiggler_enabled());
        app_screen_set_jiggler(keyboard_jiggler_enabled());
    } else if (number == 1 || number == 3) {
        if (!keyboard_queue_direction(number == 1 ? 1 : -1)) {
            ESP_LOGW("app", "keyboard unavailable or queue full");
            return;
        }
        s_last_string = number == 1 ? 1 : 2;
    }
}

static void on_knob_step(int direction)
{
    if (direction) note_activity();
    if (!direction) {
        s_combination = (lock_combination_t){0};
        if (!s_unlocked) app_screen_set_lock_ticks(0);
        return;
    }
    if (!s_unlocked) {
        if (lock_combination_step(&s_combination, direction)) {
            s_unlocked = true;
            app_screen_set_locked(false);
            haptic_click();
        } else {
            app_screen_set_lock_ticks(s_combination.progress);
        }
        return; // unlocking detents must never scroll the host
    }
    if (config_view_visible()) return;
    if (pomo_view_visible()) {
        pomo_view_turn(direction);
        return;
    }
    if (!keyboard_queue_scroll(direction)) ESP_LOGW("app", "mouse unavailable or queue full");
}
void app_inject_knob(int steps)
{
    int direction = steps > 0 ? 1 : -1;
    for (int i = 0; i < (steps > 0 ? steps : -steps); ++i) on_knob_step(direction);
}
void app_inject_vertical_swipe(bool up) { config_view_swipe(up); }
void app_inject_swipe(bool left) { pomo_view_swipe(left); }
void app_inject_tap(void) {}
void app_inject_hold(void) { pomo_view_preview_jingle(); }
void app_inject_button(int number)
{
    lv_obj_t *button = app_screen_string_button(number);
    if (!button || !lv_obj_is_visible(button) || lv_obj_has_state(button, LV_STATE_DISABLED)) return;
    lv_obj_send_event(button, LV_EVENT_PRESSED, NULL);
    lv_obj_send_event(button, LV_EVENT_SHORT_CLICKED, NULL);
}
void app_inject_reset(void)
{
    s_last_string = 0;
    lock_device();
}
void app_state_line(char *buf, size_t cap)
{
    snprintf(buf, cap, "locked=%d last=%d jiggler=%s moves=%u wheel=%d", !s_unlocked, s_last_string,
             keyboard_jiggler_enabled() ? "on" : "off", keyboard_mouse_pairs(), keyboard_wheel_total());
}
void app_init(void)
{
    if (!ui_lock(1000)) { ESP_LOGE("app", "no LVGL lock"); return; }
    app_screen_build(on_button);
    lv_obj_add_event_cb(lv_screen_active(), on_screen_activity, LV_EVENT_PRESSED, NULL);
    s_last_activity_us = esp_timer_get_time();
    lv_timer_create(idle_dim_tick, 1000, NULL);
    ui_unlock();
    knob_init(on_knob_step);
    ESP_LOGI("app", "ready: Password + Jiggler + Password 2; ring scrolls");
}
