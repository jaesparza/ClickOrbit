#pragma once
#include "lvgl.h"

// All calls run under the LVGL lock. The timer keeps time while the view is hidden.
void pomo_view_build(lv_obj_t *parent, lv_event_cb_t on_action, lv_obj_t *caption, lv_obj_t *time);
void pomo_view_show(bool visible);
bool pomo_view_visible(void);
void pomo_view_turn(int detent);
void pomo_view_swipe(bool left);
void pomo_view_preview_jingle(void);
void pomo_view_toggle(void);
void pomo_view_restart(void);
lv_obj_t *pomo_view_button(int id);
enum { POMO_OPEN = 5, POMO_TOGGLE, POMO_MENU, POMO_RESET, POMO_STATS, POMO_CLEAR, POMO_CLOCK };
