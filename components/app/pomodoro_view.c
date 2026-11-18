#include "pomodoro_view.h"
#include "pomodoro.h"
#include "pomodoro_store.h"
#include "dial_style.h"
#include "haptic.h"
#include "esp_timer.h"

static pomodoro_t session;
static lv_obj_t *panel, *arc, *phase, *clock_label, *hint;
static lv_obj_t *toggle, *menu, *reset, *toggle_text, *reset_text, *round_marks[4];
static lv_obj_t *stats, *total_label, *clear, *clear_text, *to_timer, *to_stats;
static bool visible, showing_stats, confirm_clear;
static lv_obj_t *home_caption, *home_time;
static int last_home_seconds = -1;
static int last_seconds = -1;

static int64_t now_ms(void) { return esp_timer_get_time() / 1000; }

static void render(bool controls)
{
    int seconds = (pomo_remaining(&session, now_ms()) + 999) / 1000;
    if (lv_obj_is_visible(home_time) && (controls || seconds != last_home_seconds)) {
        const char *name = session.phase == POMO_FOCUS ? "POMODORO" : "BREAK";
        const char *state = session.mode == POMO_RUNNING ? "RUNNING" :
                            session.mode == POMO_PAUSED ? "PAUSED" : "READY";
        lv_label_set_text_fmt(home_caption, "%s / %s", name, state);
        lv_label_set_text_fmt(home_time, "%02d:%02d", seconds / 60, seconds % 60);
        last_home_seconds = seconds;
    }
    if (!visible) return;
    if (showing_stats) {
        if (controls) {
            lv_obj_set_style_text_font(total_label, session.completed > 999999 ? &font_muli_28 : &font_dial_64, 0);
            lv_label_set_text_fmt(total_label, "%u", (unsigned)session.completed);
            lv_obj_set_state(clear, LV_STATE_DISABLED, session.completed == 0);
            lv_obj_set_style_bg_color(clear, lv_color_hex(confirm_clear ? DIAL_RED : DIAL_TILE), 0);
            lv_obj_set_style_text_color(clear_text,
                lv_color_hex(session.completed ? DIAL_PAPER : DIAL_MUTED), 0);
            lv_label_set_text(clear_text, confirm_clear ? "Confirm reset" : "Reset totals");
        }
        return;
    }
    if (controls) {
        const char *name = session.phase == POMO_FOCUS ? "Focus" :
                           session.phase == POMO_BREAK ? "Short break" : "Long break";
        bool is_break = session.phase != POMO_FOCUS;
        lv_label_set_text_fmt(phase, "%s   %u / 4", name, session.round);
        lv_color_t color = lv_color_hex(session.phase == POMO_FOCUS ? DIAL_ACCENT : DIAL_BREAK);
        lv_obj_set_style_text_color(phase, color, 0);
        lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(toggle, color, 0);
        for (unsigned i = 0; i < 4; ++i)
            lv_obj_set_style_bg_color(round_marks[i], i < session.round ? color : lv_color_hex(DIAL_TRACK), 0);
        lv_label_set_text(toggle_text, session.mode == POMO_RUNNING ? "Pause" :
                          session.mode == POMO_PAUSED ? "Resume" : "Start");
        lv_label_set_text(reset_text, is_break ? "Skip break" : "Reset");
        lv_obj_set_hidden(reset, !is_break && session.mode != POMO_PAUSED);
        lv_obj_set_hidden(hint, is_break || session.mode != POMO_READY);
    }
    if (controls || seconds != last_seconds) {
        lv_label_set_text_fmt(clock_label, "%02d:%02d", seconds / 60, seconds % 60);
        lv_arc_set_value(arc, seconds * 360 / (session.minutes * 60));
        last_seconds = seconds;
    }
}

static bool finish_if_due(int64_t now)
{
    uint32_t previous = session.completed;
    if (!pomo_finish(&session, now)) return false;
    if (session.completed != previous) pomo_store_save(session.completed);
    haptic_timer_alarm();
    return true;
}

static void tick(lv_timer_t *timer)
{
    (void)timer;
    bool done = finish_if_due(now_ms());
    render(done);
}

static void show_page(bool totals)
{
    showing_stats = totals;
    confirm_clear = false;
    lv_obj_set_hidden(panel, !visible || totals);
    lv_obj_set_hidden(stats, !visible || !totals);
    render(true);
}

static void on_navigation(lv_event_t *e)
{
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    if (id == POMO_CLEAR) {
        if (!session.completed) return;
        if (confirm_clear) {
            pomo_clear_completed(&session);
            pomo_store_save(0);
        }
        confirm_clear = !confirm_clear;
        render(true);
    } else {
        show_page(id == POMO_STATS);
    }
}

static void on_swipe(lv_event_t *e)
{
    lv_indev_t *input = lv_event_get_param(e);
    if (input) lv_indev_wait_release(input);
    show_page(lv_event_get_code(e) == LV_EVENT_GESTURE_LEFT);
}

static void enable_swipe(lv_obj_t *page, lv_event_code_t direction)
{
    lv_obj_set_clickable(page, true);
    lv_obj_set_gesture_bubble(page, false);
    lv_obj_add_event_cb(page, on_swipe, direction, NULL);
}

void pomo_view_swipe(bool left)
{
    if (visible) lv_obj_send_event(showing_stats ? stats : panel,
                       left ? LV_EVENT_GESTURE_LEFT : LV_EVENT_GESTURE_RIGHT, NULL);
}

static void preview_jingle(lv_event_t *event)
{
    (void)event;
    haptic_jingle();
}

void pomo_view_preview_jingle(void)
{
    if (visible && !showing_stats) lv_obj_send_event(clock_label, LV_EVENT_LONG_PRESSED, NULL);
}

void pomo_view_build(lv_obj_t *parent, lv_event_cb_t cb, lv_obj_t *caption, lv_obj_t *time)
{
    home_caption = caption;
    home_time = time;
    pomo_init(&session);
    session.completed = pomo_store_init();
    panel = dial_panel(parent);
    enable_swipe(panel, LV_EVENT_GESTURE_LEFT);
    arc = lv_arc_create(panel);
    lv_obj_set_size(arc, 334, 334);
    lv_obj_center(arc);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, 0, 360);
    lv_obj_set_clickable(arc, false);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(arc, 9, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 9, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(DIAL_TRACK), LV_PART_MAIN);

    menu = dial_button(panel, 98, 44, -52, -105, DIAL_TILE, POMO_MENU, cb);
    dial_label(menu, LV_SYMBOL_LEFT, LV_FONT_DEFAULT, DIAL_MUTED, -28, 0);
    dial_label(menu, "Menu", &font_muli_20, DIAL_PAPER, 9, 0);
    to_stats = dial_button(panel, 98, 44, 52, -105, DIAL_TILE, POMO_STATS, on_navigation);
    dial_label(to_stats, "Totals", &font_muli_20, DIAL_PAPER, -8, 0);
    dial_label(to_stats, LV_SYMBOL_RIGHT, LV_FONT_DEFAULT, DIAL_MUTED, 32, 0);
    phase = dial_label(panel, "", &font_muli_20, DIAL_ACCENT, 0, -57);
    lv_obj_set_style_text_letter_space(phase, 2, 0);
    clock_label = dial_label(panel, "25:00", &font_dial_64, DIAL_PAPER, 0, -6);
    lv_obj_set_clickable(clock_label, true);
    lv_obj_add_event_cb(clock_label, preview_jingle, LV_EVENT_LONG_PRESSED, NULL);
    for (int i = 0; i < 4; i++)
        round_marks[i] = dial_mark(panel, 17, 5, 23 * i - 34, 43, DIAL_TRACK, 2);
    toggle = dial_button(panel, 146, 48, 0, 86, DIAL_ACCENT, POMO_TOGGLE, cb);
    toggle_text = dial_label(toggle, "Start", &font_muli_28, DIAL_BG, 0, 0);
    hint = dial_label(panel, "Turn to set minutes", LV_FONT_DEFAULT, DIAL_MUTED, 0, 132);
    reset = dial_button(panel, 130, 44, 0, 135, DIAL_TILE, POMO_RESET, cb);
    reset_text = dial_label(reset, "Reset", &font_muli_20, DIAL_PAPER, 0, 0);
    stats = dial_panel(parent);
    enable_swipe(stats, LV_EVENT_GESTURE_RIGHT);
    to_timer = dial_button(stats, 110, 44, 0, -108, DIAL_TILE, POMO_CLOCK, on_navigation);
    dial_label(to_timer, LV_SYMBOL_LEFT, LV_FONT_DEFAULT, DIAL_MUTED, -30, 0);
    dial_label(to_timer, "Timer", &font_muli_20, DIAL_PAPER, 9, 0);
    dial_label(stats, "Session totals", &font_muli_28, DIAL_PAPER, 0, -55);
    total_label = dial_label(stats, "0", &font_dial_64, DIAL_ACCENT, 0, 4);
    dial_label(stats, "Completed focus sessions", &font_muli_20, DIAL_MUTED, 0, 58);
    clear = dial_button(stats, 174, 46, 0, 114, DIAL_TILE, POMO_CLEAR, on_navigation);
    clear_text = dial_label(clear, "Reset totals", &font_muli_20, DIAL_PAPER, 0, 0);
    pomo_view_show(false);
    lv_timer_create(tick, 250, NULL);
}
void pomo_view_show(bool show)
{
    visible = show;
    show_page(false);
}
bool pomo_view_visible(void) { return visible; }
void pomo_view_turn(int detent)
{
    if (showing_stats || session.mode != POMO_READY) return;
    pomo_adjust(&session, detent);
    render(true);
}
void pomo_view_toggle(void)
{
    int64_t now = now_ms();
    if (!finish_if_due(now)) pomo_toggle(&session, now);
    render(true);
}
void pomo_view_restart(void)
{
    if (!finish_if_due(now_ms())) pomo_restart(&session);
    render(true);
}
lv_obj_t *pomo_view_button(int id)
{
    if (!visible) return NULL;
    if (showing_stats) {
        if (id == POMO_CLEAR && session.completed) return clear;
        return id == POMO_CLOCK ? to_timer : NULL;
    }
    if (id == POMO_STATS) return to_stats;
    if (id == POMO_TOGGLE) return toggle;
    if (id == POMO_MENU) return menu;
    if (id == POMO_RESET && (session.phase != POMO_FOCUS || session.mode == POMO_PAUSED)) return reset;
    return NULL;
}
