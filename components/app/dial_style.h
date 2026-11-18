#pragma once
#include "lvgl.h"
#include "fonts.h"

// Shared materials and widgets for the three round instrument faces.
#define DIAL_BG       0x101423
#define DIAL_TILE     0x232C43
#define DIAL_TRACK    0x384462
#define DIAL_PAPER    0xF3F5FF
#define DIAL_MUTED    0xB5C4DE
#define DIAL_ACCENT   0xD6F475
#define DIAL_RED      0x71342E
#define DIAL_CORAL    0xFFAA96
#define DIAL_BREAK    0x9AD7FF

lv_obj_t *dial_panel(lv_obj_t *parent);
lv_obj_t *dial_label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                     unsigned color, int x, int y);
lv_obj_t *dial_mark(lv_obj_t *parent, int w, int h, int x, int y, unsigned color, int radius);
lv_obj_t *dial_button(lv_obj_t *parent, int w, int h, int x, int y, unsigned color,
                      int id, lv_event_cb_t callback);
