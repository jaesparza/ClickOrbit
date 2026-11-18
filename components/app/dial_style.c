#include "dial_style.h"
#include "tokens.h"

static void place(lv_obj_t *o, int w, int h, int x, int y)
{
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_align(o, LV_ALIGN_CENTER, x, y);
    lv_obj_set_scrollable(o, false);
}

lv_obj_t *dial_panel(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    place(o, HUD_PANEL_DIA, HUD_PANEL_DIA, 0, 0);
    lv_obj_set_clickable(o, false);
    return o;
}

lv_obj_t *dial_label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                     unsigned color, int x, int y)
{
    lv_obj_t *o = lv_label_create(parent);
    lv_obj_set_style_text_font(o, font, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(color), 0);
    lv_label_set_text(o, text);
    lv_obj_align(o, LV_ALIGN_CENTER, x, y);
    lv_obj_set_clickable(o, false);
    return o;
}

lv_obj_t *dial_mark(lv_obj_t *parent, int w, int h, int x, int y, unsigned color, int radius)
{
    lv_obj_t *o = lv_obj_create(parent);
    place(o, w, h, x, y);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_clickable(o, false);
    return o;
}

lv_obj_t *dial_button(lv_obj_t *parent, int w, int h, int x, int y, unsigned color,
                      int id, lv_event_cb_t cb)
{
    lv_obj_t *o = lv_button_create(parent);
    place(o, w, h, x, y);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_70, LV_STATE_PRESSED);
    lv_obj_set_style_radius(o, 18, 0);
    lv_obj_add_event_cb(o, cb, LV_EVENT_SHORT_CLICKED, (void *)(intptr_t)id);
    return o;
}
