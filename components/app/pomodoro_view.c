#include "pomodoro_view.h"
#include "pomodoro.h"
#include "dial_style.h"
#include "haptic.h"
#include "esp_timer.h"

static pomodoro_t session;
static lv_obj_t *panel, *arc, *phase, *clock_label, *hint;
static lv_obj_t *toggle, *menu, *reset, *toggle_text, *reset_text, *round_marks[4];
static bool visible;
static int last_seconds = -1;

static int64_t now_ms(void) { return esp_timer_get_time() / 1000; }

static void render(bool controls)
{
    if (!visible) return;
    int seconds = (pomo_remaining(&session, now_ms()) + 999) / 1000;
    if (controls) {
        const char *name = session.phase == POMO_FOCUS ? "Focus" :
                           session.phase == POMO_BREAK ? "Short break" : "Long break";
        bool is_break = session.phase != POMO_FOCUS;
        lv_label_set_text(phase, name);
        lv_color_t color = lv_color_hex(session.phase == POMO_FOCUS ? DIAL_ORANGE : DIAL_BREAK);
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

static void tick(lv_timer_t *timer)
{
    (void)timer;
    bool done = pomo_finish(&session, now_ms());
    if (done) haptic_timer_alarm();
    render(done);
}

static void preview_jingle(lv_event_t *event)
{
    (void)event;
    haptic_jingle();
}

void pomo_view_preview_jingle(void)
{
    if (visible) lv_obj_send_event(clock_label, LV_EVENT_LONG_PRESSED, NULL);
}

void pomo_view_build(lv_obj_t *parent, lv_event_cb_t cb)
{
    pomo_init(&session);
    panel = dial_panel(parent);
    arc = lv_arc_create(panel);
    lv_obj_set_size(arc, 334, 334);
    lv_obj_center(arc);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, 0, 360);
    lv_obj_set_clickable(arc, false);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(arc, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(DIAL_TRACK), LV_PART_MAIN);

    menu = dial_button(panel, 98, 44, 0, -110, DIAL_TILE, POMO_MENU, cb);
    dial_label(menu, LV_SYMBOL_LEFT, LV_FONT_DEFAULT, DIAL_MUTED, -28, 0);
    dial_label(menu, "Menu", &font_muli_20, DIAL_PAPER, 9, 0);
    phase = dial_label(panel, "", &font_muli_20, DIAL_ORANGE, 0, -57);
    lv_obj_set_style_text_letter_space(phase, 2, 0);
    clock_label = dial_label(panel, "25:00", &font_dial_64, DIAL_PAPER, 0, -6);
    lv_obj_set_clickable(clock_label, true);
    lv_obj_add_event_cb(clock_label, preview_jingle, LV_EVENT_LONG_PRESSED, NULL);
    for (int i = 0; i < 4; i++)
        round_marks[i] = dial_mark(panel, 17, 5, 23 * i - 34, 43, DIAL_TRACK, 2);
    toggle = dial_button(panel, 146, 48, 0, 86, DIAL_ORANGE, POMO_TOGGLE, cb);
    toggle_text = dial_label(toggle, "Start", &font_muli_28, DIAL_BG, 0, 0);
    hint = dial_label(panel, "Turn to set minutes", LV_FONT_DEFAULT, DIAL_MUTED, 0, 132);
    reset = dial_button(panel, 130, 44, 0, 135, DIAL_TILE, POMO_RESET, cb);
    reset_text = dial_label(reset, "Reset", &font_muli_20, DIAL_PAPER, 0, 0);
    pomo_view_show(false);
    lv_timer_create(tick, 250, NULL);
}
void pomo_view_show(bool show)
{
    visible = show;
    lv_obj_set_hidden(panel, !show);
    render(true);
}
bool pomo_view_visible(void) { return visible; }
void pomo_view_turn(int detent)
{
    if (session.mode != POMO_READY) return;
    pomo_adjust(&session, detent);
    render(true);
}
void pomo_view_toggle(void)
{
    int64_t now = now_ms();
    if (pomo_finish(&session, now)) haptic_timer_alarm();
    else {
        pomo_toggle(&session, now);
        haptic_click();
    }
    render(true);
}
void pomo_view_restart(void)
{
    if (!pomo_finish(&session, now_ms())) pomo_restart(&session);
    render(true);
}
lv_obj_t *pomo_view_button(int id)
{
    if (id == POMO_TOGGLE) return toggle;
    if (id == POMO_MENU) return menu;
    if (id == POMO_RESET && (session.phase != POMO_FOCUS || session.mode == POMO_PAUSED)) return reset;
    return NULL;
}
