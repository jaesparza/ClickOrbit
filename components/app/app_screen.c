// Menu buttons within the round panel's safe circle. All calls hold the LVGL lock.
#include "app_screen.h"
#include "tokens.h"
#include "fonts.h"

static lv_obj_t *s_buttons[4], *s_jiggler_label;
static lv_obj_t *s_menu, *s_locked_label;
static void make_button(lv_obj_t *scr, int index, int row, lv_event_cb_t callback)
{
    lv_obj_t *button = lv_button_create(scr);
    s_buttons[index] = button;
    lv_obj_set_size(button, 210, 54);
    lv_obj_align(button, LV_ALIGN_CENTER, 0, (2 * row - 3) * 33);
    lv_obj_set_style_pad_all(button, 0, 0);
    lv_obj_set_style_radius(button, 20, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(HUD_KEY_PANEL), 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(HUD_KEY_ORANGE), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(button, lv_color_hex(HUD_KEY_ORANGE), 0);
    lv_obj_set_style_border_width(button, 2, 0);
    lv_obj_set_scrollable(button, false);
    lv_obj_add_event_cb(button, callback, LV_EVENT_SHORT_CLICKED, (void *)(intptr_t)(index + 1));
    lv_obj_t *label = lv_label_create(button);
    lv_obj_set_style_text_font(label, &font_muli_28, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(HUD_INK), 0);
    static const char *const names[] = {"Ambu", "Jiggler off", "Home", "Lock"};
    lv_label_set_text(label, names[index]);
    if (index == 1) s_jiggler_label = label;
    lv_obj_center(label);
    lv_obj_set_clickable(label, false);
}
void app_screen_build(lv_event_cb_t on_button)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(HUD_KEY_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(scr, false);
    lv_obj_set_clickable(scr, false);
    s_menu = lv_obj_create(scr);
    lv_obj_remove_style_all(s_menu);
    lv_obj_set_size(s_menu, HUD_PANEL_DIA, HUD_PANEL_DIA);
    lv_obj_set_scrollable(s_menu, false);
    lv_obj_set_clickable(s_menu, false);
    make_button(s_menu, 3, 0, on_button);
    make_button(s_menu, 0, 1, on_button);
    make_button(s_menu, 1, 2, on_button);
    make_button(s_menu, 2, 3, on_button);
    s_locked_label = lv_label_create(scr);
    lv_obj_set_style_text_font(s_locked_label, &font_muli_28, 0);
    lv_obj_set_style_text_color(s_locked_label, lv_color_hex(HUD_KEY_ORANGE), 0);
    lv_obj_set_style_text_align(s_locked_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(s_locked_label);
    app_screen_set_jiggler(false);
    app_screen_set_locked(true);
}
void app_screen_set_locked(bool locked)
{
    if (locked) app_screen_set_lock_ticks(0);
    lv_obj_set_hidden(s_menu, locked);
    lv_obj_set_hidden(s_locked_label, !locked);
}
void app_screen_set_lock_ticks(unsigned ticks)
{
    lv_label_set_text_fmt(s_locked_label, "Locked\n%u", ticks);
}
void app_screen_set_jiggler(bool enabled)
{
    lv_label_set_text(s_jiggler_label, enabled ? "Jiggler on" : "Jiggler off");
    lv_color_t color = lv_color_hex(enabled ? HUD_KEY_ORANGE : HUD_REC);
    lv_obj_set_style_bg_color(s_buttons[1], color, 0);
    lv_obj_set_style_bg_color(s_buttons[1], color, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(s_buttons[1], color, 0);
    lv_obj_set_style_text_color(s_jiggler_label, lv_color_hex(enabled ? HUD_KEY_BG : HUD_INK), 0);
}
lv_obj_t *app_screen_string_button(int number)
{
    return number >= 1 && number <= 4 ? s_buttons[number - 1] : NULL;
}
