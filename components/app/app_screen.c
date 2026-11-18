#include "app_screen.h"
#include "dial_style.h"
#include "pomodoro_view.h"

static lv_obj_t *buttons[5], *jiggler_label, *switch_track, *switch_thumb;
static lv_obj_t *menu, *lock_face, *count, *progress[10];

static void lock_icon(lv_obj_t *parent, int x, int y, unsigned color)
{
    lv_obj_t *bow = dial_mark(parent, 16, 19, x, y - 3, color, 8);
    lv_obj_set_style_bg_opa(bow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bow, 3, 0);
    lv_obj_set_style_border_color(bow, lv_color_hex(color), 0);
    dial_mark(parent, 23, 15, x, y + 6, color, 4);
    dial_mark(parent, 3, 6, x, y + 6, DIAL_BG, 2);
}

static void build_lock(lv_obj_t *root)
{
    lock_face = dial_panel(root);
    lock_icon(lock_face, 0, -110, DIAL_ORANGE);
    dial_label(lock_face, "Locked", &font_muli_28, DIAL_PAPER, 0, -65);
    count = dial_label(lock_face, "0", &font_dial_64, DIAL_PAPER, 0, 3);
    // Five teeth on each side of the centre gap echo the two encoder strokes.
    for (int i = 0; i < 10; ++i) {
        int x = (i * 20) - 90 + (i < 5 ? -4 : 4);
        progress[i] = dial_mark(lock_face, 9, 22, x, 67, DIAL_TRACK, 4);
    }
    dial_label(lock_face, "Turn to unlock", &font_muli_20, DIAL_MUTED, 0, 112);
}

static lv_obj_t *tile(int id, int row, int col, const char *name, const char *icon,
                      lv_event_cb_t cb)
{
    lv_obj_t *o = dial_button(menu, 119, 65, col * 63, row ? 76 : 5,
                              id == 2 ? DIAL_RED : DIAL_TILE, id, cb);
    buttons[id - 1] = o;
    if (icon) dial_label(o, icon, LV_FONT_DEFAULT, DIAL_MUTED, 0, -14);
    lv_obj_t *title = dial_label(o, name, &font_muli_20, DIAL_PAPER, 0, 12);
    if (id == 2) jiggler_label = title;
    return o;
}

void app_screen_build(lv_event_cb_t cb)
{
    lv_obj_t *root = lv_screen_active();
    lv_obj_set_style_bg_color(root, lv_color_hex(DIAL_BG), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(root, false);
    menu = dial_panel(root);
    lv_obj_t *hero = dial_button(menu, 232, 68, 0, -71, DIAL_PAPER, POMO_OPEN, cb);
    buttons[POMO_OPEN - 1] = hero;
    // Miniature stopwatch on the timer launcher.
    dial_mark(hero, 9, 6, -82, -18, DIAL_BG, 2);
    dial_mark(hero, 34, 34, -82, 1, DIAL_BG, 17);
    dial_mark(hero, 3, 12, -82, -3, DIAL_ORANGE, 1);
    dial_mark(hero, 8, 3, -79, 2, DIAL_ORANGE, 1);
    dial_label(hero, "Pomodoro", &font_muli_28, DIAL_BG, 7, 0);
    dial_label(hero, LV_SYMBOL_RIGHT, LV_FONT_DEFAULT, DIAL_BG, 97, 0);
    lv_obj_t *lock = tile(4, 0, -1, "Lock", NULL, cb);
    lock_icon(lock, 0, -14, DIAL_MUTED);
    lv_obj_t *jiggler = tile(2, 0, 1, "Jiggler off", NULL, cb);
    switch_track = dial_mark(jiggler, 32, 14, 0, -14, DIAL_CORAL, 7);
    switch_thumb = dial_mark(switch_track, 10, 10, -8, 0, DIAL_BG, 5);
    tile(1, 1, -1, "Ambu", LV_SYMBOL_KEYBOARD, cb);
    tile(3, 1, 1, "Home", LV_SYMBOL_HOME, cb);
    build_lock(root);
    pomo_view_build(root, cb);
    app_screen_set_jiggler(false);
    app_screen_set_locked(true);
}
void app_screen_set_locked(bool locked)
{
    pomo_view_show(false);
    if (locked) app_screen_set_lock_ticks(0);
    lv_obj_set_hidden(menu, locked);
    lv_obj_set_hidden(lock_face, !locked);
}
void app_screen_show_pomodoro(bool show)
{
    lv_obj_set_hidden(menu, show);
    pomo_view_show(show);
}
void app_screen_set_lock_ticks(unsigned ticks)
{
    lv_label_set_text_fmt(count, "%u", ticks);
    for (unsigned i = 0; i < 10; i++)
        lv_obj_set_style_bg_color(progress[i], lv_color_hex(i < ticks ? DIAL_ORANGE : DIAL_TRACK), 0);
}
void app_screen_set_jiggler(bool on)
{
    lv_label_set_text(jiggler_label, on ? "Jiggler on" : "Jiggler off");
    lv_obj_set_style_bg_color(buttons[1], lv_color_hex(on ? DIAL_ORANGE : DIAL_RED), 0);
    lv_obj_set_style_text_color(jiggler_label, lv_color_hex(on ? DIAL_BG : DIAL_PAPER), 0);
    lv_obj_set_style_bg_color(switch_track, lv_color_hex(on ? DIAL_BG : DIAL_CORAL), 0);
    lv_obj_set_style_bg_color(switch_thumb, lv_color_hex(on ? DIAL_ORANGE : DIAL_BG), 0);
    lv_obj_align(switch_thumb, LV_ALIGN_CENTER, on ? 8 : -8, 0);
}
lv_obj_t *app_screen_string_button(int number)
{
    return number >= 1 && number <= 5 ? buttons[number - 1] : pomo_view_button(number);
}
