#include "pomodoro_view.h"
#include "pomodoro.h"
#include "fonts.h"
#include "tokens.h"
#include "haptic.h"
#include "esp_timer.h"

static pomodoro_t session;
static lv_obj_t *panel, *arc, *phase, *clock_label, *hint;
static lv_obj_t *toggle, *menu, *reset;
static bool visible;
static int last_seconds = -1;

static int64_t now_ms(void) { return esp_timer_get_time() / 1000; }

static lv_obj_t *label(lv_obj_t *parent, int y, const lv_font_t *font)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(HUD_INK), 0);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, y);
    return obj;
}

static lv_obj_t *button(int y, int width, int id, const char *name, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(panel);
    lv_obj_set_size(btn, width, 46);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, y);
    lv_obj_set_style_radius(btn, 23, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(HUD_KEY_PANEL), 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_SHORT_CLICKED, (void *)(intptr_t)id);
    lv_label_set_text(label(btn, 0, &font_muli_28), name);
    return btn;
}

static void render(bool controls)
{
    if (!visible) return;
    int seconds = (pomo_remaining(&session, now_ms()) + 999) / 1000;
    if (controls) {
        const char *name = session.phase == POMO_FOCUS ? "Focus" :
                           session.phase == POMO_BREAK ? "Short break" : "Long break";
        unsigned round = (session.completed + (session.phase == POMO_FOCUS ? 0 : 3)) % 4 + 1;
        lv_label_set_text_fmt(phase, "%s  %u/4", name, round);
        lv_color_t color = lv_color_hex(session.phase == POMO_FOCUS ? HUD_KEY_ORANGE : HUD_GREEN);
        lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(toggle, color, 0);
        lv_label_set_text(lv_obj_get_child(toggle, 0), session.mode == POMO_RUNNING ? "Pause" :
                          session.mode == POMO_PAUSED ? "Resume" : "Start");
        lv_obj_set_hidden(reset, session.mode != POMO_PAUSED);
        lv_obj_set_hidden(hint, session.mode != POMO_READY);
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
    panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, HUD_PANEL_DIA, HUD_PANEL_DIA);
    lv_obj_center(panel);
    lv_obj_set_scrollable(panel, false);
    arc = lv_arc_create(panel);
    lv_obj_set_size(arc, 318, 318);
    lv_obj_center(arc);
    lv_arc_set_rotation(arc, 100);
    lv_arc_set_bg_angles(arc, 0, 340);
    lv_arc_set_range(arc, 0, 360);
    lv_obj_set_clickable(arc, false);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_color(arc, lv_color_hex(HUD_OFF), LV_PART_MAIN);
    menu = button(-104, 110, POMO_MENU, "Menu", cb);
    phase = label(panel, -51, &font_muli_28);
    clock_label = label(panel, -4, &font_muli_28);
    lv_obj_set_clickable(clock_label, true);
    lv_obj_add_event_cb(clock_label, preview_jingle, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_set_style_transform_scale(clock_label, 400, 0);
    lv_obj_set_style_transform_pivot_x(clock_label, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(clock_label, LV_PCT(50), 0);
    toggle = button(58, 148, POMO_TOGGLE, "Start", cb);
    lv_obj_set_style_text_color(lv_obj_get_child(toggle, 0), lv_color_hex(HUD_KEY_BG), 0);
    hint = label(panel, 108, LV_FONT_DEFAULT);
    lv_label_set_text(hint, "Turn to set minutes");
    reset = button(111, 110, POMO_RESET, "Reset", cb);
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
    pomo_restart(&session);
    render(true);
}
lv_obj_t *pomo_view_button(int id)
{
    if (id == POMO_TOGGLE) return toggle;
    if (id == POMO_MENU) return menu;
    if (id == POMO_RESET && session.mode == POMO_PAUSED) return reset;
    return NULL;
}
