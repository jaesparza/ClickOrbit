#pragma once
#include "lvgl.h"

enum { CONFIG_BACK = 12, CONFIG_OFFICE, CONFIG_PREVIEW };
// All calls hold the LVGL lock. Build after loading preferences.
void config_view_build(lv_obj_t *root, lv_obj_t *home);
bool config_view_visible(void);
void config_view_hide(void);
void config_view_swipe(bool up);
lv_obj_t *config_view_button(int id);
