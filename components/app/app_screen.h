#pragma once
// Private widgets. Caller holds the LVGL lock; no networking/USB I/O here.
// Widgets live for the app lifetime; callback gets number 1/2/3/4 as user data.
#include "lvgl.h"
void app_screen_build(lv_event_cb_t on_button);
void app_screen_set_jiggler(bool enabled);
void app_screen_set_locked(bool locked);
void app_screen_show_pomodoro(bool show);
void app_screen_set_lock_ticks(unsigned ticks);
lv_obj_t *app_screen_string_button(int number);
