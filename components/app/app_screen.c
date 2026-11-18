#include "app_screen.h"
#include "dial_style.h"
#include "config_view.h"
#include "pomodoro_view.h"

static lv_obj_t *buttons[5], *jiggler_label, *lock_face, *menu;
static lv_obj_t *count, *progress[10], *shortcut_labels[2];
static lv_timer_t *feedback_timer;

static void clear_feedback(lv_timer_t *timer)
{
    lv_label_set_text(shortcut_labels[0], "Work");
    lv_label_set_text(shortcut_labels[1], "Home");
    lv_timer_pause(timer);
}

void app_screen_shortcut_feedback(int number, bool queued)
{
    clear_feedback(feedback_timer);
    lv_label_set_text(shortcut_labels[number == 1 ? 0 : 1], queued ? "Queued" : "Try again");
    lv_timer_reset(feedback_timer);
    lv_timer_resume(feedback_timer);
}

static void build_lock(lv_obj_t *root)
{
    lock_face = dial_panel(root);
    lv_obj_t *shackle = dial_mark(lock_face, 16, 18, 0, -100, DIAL_BG, 8);
    lv_obj_set_style_border_width(shackle, 3, 0);
    lv_obj_set_style_border_color(shackle, lv_color_hex(DIAL_ACCENT), 0);
    dial_mark(lock_face, 24, 16, 0, -90, DIAL_ACCENT, 4);
    dial_label(lock_face, "Locked", &font_muli_28, DIAL_PAPER, 0, -57);
    count = dial_label(lock_face, "0", &font_dial_64, DIAL_PAPER, 0, 0);
    for (int i = 0; i < 10; ++i) {
        int angle = (135 + i * 30) % 360;
        int x = 143 * lv_trigo_cos(angle) / LV_TRIGO_SIN_MAX;
        int y = 143 * lv_trigo_sin(angle) / LV_TRIGO_SIN_MAX;
        progress[i] = dial_mark(lock_face, 13, 13, x, y, DIAL_TRACK, 7);
    }
    dial_label(lock_face, "Turn the combination", &font_muli_20, DIAL_MUTED, 0, 75);
}

void app_screen_build(lv_event_cb_t cb)
{
    lv_obj_t *root = lv_screen_active();
    lv_obj_set_style_bg_color(root, lv_color_hex(DIAL_BG), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(root, false);
    menu = dial_panel(root);

    lv_obj_t *focus = dial_button(menu, 242, 92, 0, -66, DIAL_ACCENT, POMO_OPEN, cb);
    buttons[POMO_OPEN - 1] = focus;
    lv_obj_t *caption = dial_label(focus, "POMODORO", LV_FONT_DEFAULT, DIAL_BG, 0, -30);
    lv_obj_t *time = dial_label(focus, "25:00", &font_dial_64, DIAL_BG, 0, 10);
    dial_label(focus, LV_SYMBOL_RIGHT, LV_FONT_DEFAULT, DIAL_BG, 103, 8);
    for (int i = 0; i < 2; ++i) {
        int id = i == 0 ? 1 : 3;
        lv_obj_t *key = dial_button(menu, 136, 62, i == 0 ? -71 : 71, 21, DIAL_TILE, id, cb);
        buttons[id - 1] = key;
        dial_label(key, LV_SYMBOL_KEYBOARD, LV_FONT_DEFAULT, DIAL_MUTED, 0, -14);
        shortcut_labels[i] = dial_label(key, i == 0 ? "Work" : "Home", &font_muli_20, DIAL_PAPER, 0, 10);
    }
    buttons[1] = dial_button(menu, 147, 46, -46, 84, DIAL_TILE, 2, cb);
    jiggler_label = dial_label(buttons[1], "", &font_muli_20, DIAL_PAPER, 0, 0);
    buttons[3] = dial_button(menu, 78, 46, 74, 84, DIAL_TILE, 4, cb);
    dial_label(buttons[3], "Lock", &font_muli_20, DIAL_PAPER, 0, 0);
    feedback_timer = lv_timer_create(clear_feedback, 1100, NULL);
    lv_timer_pause(feedback_timer);
    build_lock(root);
    pomo_view_build(root, cb, caption, time);
    config_view_build(root, menu);
    app_screen_set_jiggler(false);
    app_screen_set_locked(true);
}
void app_screen_set_locked(bool locked)
{
    config_view_hide();
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
        lv_obj_set_style_bg_color(progress[i], lv_color_hex(i < ticks ? DIAL_ACCENT : DIAL_TRACK), 0);
}
void app_screen_set_jiggler(bool on)
{
    lv_label_set_text(jiggler_label, on ? "Jiggler on" : "Jiggler off");
    lv_obj_set_style_bg_color(buttons[1], lv_color_hex(on ? DIAL_ACCENT : DIAL_TILE), 0);
    lv_obj_set_style_text_color(jiggler_label, lv_color_hex(on ? DIAL_BG : DIAL_PAPER), 0);
}
lv_obj_t *app_screen_string_button(int number)
{
    if (number >= CONFIG_BACK || config_view_visible()) return config_view_button(number);
    return number >= 1 && number <= 5 ? buttons[number - 1] : pomo_view_button(number);
}
