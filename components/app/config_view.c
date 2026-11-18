#include "config_view.h"
#include "dial_style.h"
#include "haptic.h"
#include "pomodoro_store.h"

static lv_obj_t *panel, *home, *back, *toggle, *state, *preview;
static bool visible;

static void display_mode(void)
{
    bool enabled = pomo_store_office();
    lv_label_set_text(state, enabled ? "On" : "Off");
    lv_obj_set_style_text_color(state, lv_color_hex(enabled ? DIAL_ORANGE : DIAL_MUTED), 0);
}

void config_view_hide(void)
{
    lv_obj_set_hidden(panel, true);
    visible = false;
}

static void navigate(bool open)
{
    visible = open;
    lv_obj_set_hidden(home, open);
    lv_obj_set_hidden(panel, !open);
}

static void on_swipe(lv_event_t *event)
{
    lv_indev_t *input = lv_event_get_param(event);
    if (input) lv_indev_wait_release(input);
    navigate(lv_event_get_code(event) == LV_EVENT_GESTURE_UP);
}

static void watch_swipe(lv_obj_t *page, lv_event_code_t direction)
{
    lv_obj_set_clickable(page, true);
    lv_obj_set_gesture_bubble(page, false);
    lv_obj_add_event_cb(page, on_swipe, direction, NULL);
}

static void on_action(lv_event_t *event)
{
    int id = (int)(intptr_t)lv_event_get_user_data(event);
    if (id == CONFIG_BACK) navigate(false);
    else if (id == CONFIG_PREVIEW) haptic_timer_alarm();
    else if (id == CONFIG_OFFICE) {
        bool enabled = !pomo_store_office();
        haptic_set_office_mode(enabled);
        pomo_store_set_office(enabled);
        display_mode();
    }
}

void config_view_build(lv_obj_t *root, lv_obj_t *home_page)
{
    home = home_page;
    watch_swipe(home, LV_EVENT_GESTURE_UP);
    dial_label(home, LV_SYMBOL_UP "  Settings", LV_FONT_DEFAULT, DIAL_MUTED, 0, 136);
    panel = dial_panel(root);
    watch_swipe(panel, LV_EVENT_GESTURE_DOWN);
    back = dial_button(panel, 110, 44, 0, -110, DIAL_TILE, CONFIG_BACK, on_action);
    dial_label(back, LV_SYMBOL_DOWN "  Menu", LV_FONT_DEFAULT, DIAL_PAPER, 0, 0);
    dial_label(panel, "Configuration", &font_muli_28, DIAL_PAPER, 0, -54);
    toggle = dial_button(panel, 246, 54, 0, 4, DIAL_TILE, CONFIG_OFFICE, on_action);
    dial_label(toggle, "Office mode", &font_muli_20, DIAL_PAPER, -36, 0);
    state = dial_label(toggle, "", &font_muli_20, DIAL_MUTED, 82, 0);
    dial_label(panel, "Office mode mutes clicks\nand softens alerts.", LV_FONT_DEFAULT, DIAL_MUTED, 0, 63);
    preview = dial_button(panel, 166, 44, 0, 118, DIAL_TILE, CONFIG_PREVIEW, on_action);
    dial_label(preview, "Test alert", &font_muli_20, DIAL_PAPER, 0, 0);
    haptic_set_office_mode(pomo_store_office());
    display_mode();
    config_view_hide();
}
bool config_view_visible(void) { return visible; }

void config_view_swipe(bool up)
{
    lv_obj_t *page = up ? home : panel;
    if (lv_obj_is_visible(page))
        lv_obj_send_event(page, up ? LV_EVENT_GESTURE_UP : LV_EVENT_GESTURE_DOWN, NULL);
}

lv_obj_t *config_view_button(int id)
{
    if (id == CONFIG_OFFICE) return toggle;
    if (id == CONFIG_BACK) return back;
    if (id == CONFIG_PREVIEW) return preview;
    return NULL;
}
