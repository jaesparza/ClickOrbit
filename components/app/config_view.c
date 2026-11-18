#include "config_view.h"
#include "dial_style.h"
#include "haptic.h"
#include "pomodoro_store.h"
#include "keyboard.h"
#include "keyboard_ble.h"
#include "battery.h"
#include "backlight.h"

static lv_obj_t *panel, *home, *back, *toggle, *state, *preview;
static lv_obj_t *interval_prev, *interval_next, *interval_label;
static lv_obj_t *open_settings, *open_status, *open_display;
static bool visible;
static lv_obj_t *status_panel, *battery_label, *connection_label;
static lv_obj_t *display_panel, *light_test, *status_back, *display_back;
static bool dimmed, previewing;
static uint32_t preview_start;
static const uint32_t intervals_ms[] = {60000, 300000, 600000, 1800000, 2000};
static const char *const interval_names[] = {"1 min", "5 min", "10 min", "30 min", "Test 2 s"};

static unsigned selected_interval(void)
{
    uint32_t current = pomo_store_jiggler_interval_ms();
    for (unsigned i = 0; i < sizeof(intervals_ms) / sizeof(intervals_ms[0]); ++i)
        if (intervals_ms[i] == current) return i;
    return 1;
}

static void display_interval(void)
{
    lv_label_set_text(interval_label, interval_names[selected_interval()]);
    lv_color_t color = selected_interval() == 4 ? lv_color_hex(DIAL_ACCENT) : lv_color_hex(DIAL_PAPER);
    lv_obj_set_style_text_color(interval_label, color, 0);
}

static void display_mode(void)
{
    bool enabled = pomo_store_office();
    lv_label_set_text(state, enabled ? "On" : "Off");
    lv_obj_set_style_text_color(state, lv_color_hex(enabled ? DIAL_ACCENT : DIAL_MUTED), 0);
}

void config_view_hide(void)
{
    lv_obj_set_hidden(panel, true);
    lv_obj_set_hidden(status_panel, true);
    lv_obj_set_hidden(display_panel, true);
    previewing = false;
    visible = false;
}

static void refresh_status(lv_timer_t *timer)
{
    (void)timer;
    if (!status_panel || !lv_obj_is_visible(status_panel)) return;
    int percent = battery_estimate();
    if (percent >= 0) lv_label_set_text_fmt(battery_label, "%d%%", percent);
    else lv_label_set_text(battery_label, "Unavailable");
    bool connected = keyboard_ble_connected();
    lv_label_set_text(connection_label, connected ? "Connected" : "Disconnected");
    lv_obj_set_style_text_color(connection_label, lv_color_hex(connected ? DIAL_BREAK : DIAL_ACCENT), 0);
}

static void show(lv_obj_t *destination)
{
    lv_obj_set_hidden(home, destination != home);
    lv_obj_set_hidden(status_panel, destination != status_panel);
    lv_obj_set_hidden(panel, destination != panel);
    lv_obj_set_hidden(display_panel, destination != display_panel);
    visible = destination != home;
    previewing = false;
    if (destination == status_panel) refresh_status(NULL);
}

static void light_tick(lv_timer_t *timer)
{
    (void)timer;
    if (previewing && lv_tick_elaps(preview_start) >= 3000) previewing = false;
    bool low = previewing || lv_display_get_inactive_time(NULL) >= 300000;
    if (dimmed != low) {
        dimmed = low;
        backlight_set_percent(low ? 20 : 80);
    }
}

static void on_swipe(lv_event_t *event)
{
    lv_indev_t *input = lv_event_get_param(event);
    if (input) lv_indev_wait_release(input);
    lv_obj_t *page = lv_event_get_current_target(event);
    bool up = lv_event_get_code(event) == LV_EVENT_GESTURE_UP;
    if (page == home) show(up ? panel : status_panel);
    else if (page == panel) show(up ? display_panel : home);
    else if (page == display_panel) show(panel);
    else show(home);
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
    if (id == CONFIG_BACK) show(home);
    else if (id == CONFIG_OPEN) show(panel);
    else if (id == CONFIG_STATUS) show(status_panel);
    else if (id == CONFIG_DISPLAY) show(display_panel);
    else if (id == CONFIG_DIM_TEST) {
        lv_display_trigger_activity(NULL);
        preview_start = lv_tick_get();
        previewing = true;
        light_tick(NULL);
    }
    else if (id == CONFIG_PREVIEW) haptic_timer_alarm();
    else if (id == CONFIG_OFFICE) {
        bool enabled = !pomo_store_office();
        haptic_set_office_mode(enabled);
        pomo_store_set_office(enabled);
        display_mode();
    } else if (id == CONFIG_INTERVAL_PREV || id == CONFIG_INTERVAL_NEXT) {
        unsigned count = sizeof(intervals_ms) / sizeof(intervals_ms[0]);
        unsigned selected = selected_interval();
        selected = id == CONFIG_INTERVAL_NEXT ? (selected + 1) % count : (selected + count - 1) % count;
        pomo_store_set_jiggler_interval_ms(intervals_ms[selected]);
        keyboard_set_jiggler_interval_ms(intervals_ms[selected]);
        display_interval();
    }
}

static lv_obj_t *nav(lv_obj_t *page, const char *title, int y, int action)
{
    lv_obj_t *button = dial_button(page, 152, 40, 0, y, DIAL_BG, action, on_action);
    dial_label(button, title, LV_FONT_DEFAULT, DIAL_MUTED, 0, 0);
    return button;
}

void config_view_build(lv_obj_t *root, lv_obj_t *home_page)
{
    home = home_page;
    watch_swipe(home, LV_EVENT_GESTURE_UP);
    watch_swipe(home, LV_EVENT_GESTURE_DOWN);
    open_status = nav(home, LV_SYMBOL_DOWN "  Notifications", -139, CONFIG_STATUS);
    open_settings = nav(home, LV_SYMBOL_UP "  Settings", 139, CONFIG_OPEN);
    panel = dial_panel(root);
    watch_swipe(panel, LV_EVENT_GESTURE_DOWN);
    watch_swipe(panel, LV_EVENT_GESTURE_UP);
    back = nav(panel, LV_SYMBOL_DOWN "  Menu", -139, CONFIG_BACK);
    dial_label(panel, "Settings", &font_muli_28, DIAL_PAPER, 0, -95);
    toggle = dial_button(panel, 246, 50, 0, -46, DIAL_TILE, CONFIG_OFFICE, on_action);
    dial_label(toggle, "Office mode", &font_muli_20, DIAL_PAPER, -36, 0);
    state = dial_label(toggle, "", &font_muli_20, DIAL_MUTED, 82, 0);
    dial_label(panel, "Jiggler interval", &font_muli_20, DIAL_MUTED, 0, 3);
    interval_prev = dial_button(panel, 48, 40, -91, 40, DIAL_TILE, CONFIG_INTERVAL_PREV, on_action);
    dial_label(interval_prev, LV_SYMBOL_LEFT, LV_FONT_DEFAULT, DIAL_PAPER, 0, 0);
    interval_label = dial_label(panel, "", &font_muli_20, DIAL_PAPER, 0, 40);
    interval_next = dial_button(panel, 48, 40, 91, 40, DIAL_TILE, CONFIG_INTERVAL_NEXT, on_action);
    dial_label(interval_next, LV_SYMBOL_RIGHT, LV_FONT_DEFAULT, DIAL_PAPER, 0, 0);
    preview = dial_button(panel, 166, 40, 0, 92, DIAL_TILE, CONFIG_PREVIEW, on_action);
    dial_label(preview, "Test alert", &font_muli_20, DIAL_PAPER, 0, 0);
    haptic_set_office_mode(pomo_store_office());
    keyboard_set_jiggler_interval_ms(pomo_store_jiggler_interval_ms());
    display_mode();
    display_interval();
    status_panel = dial_panel(root);
    watch_swipe(status_panel, LV_EVENT_GESTURE_UP);
    open_display = nav(panel, LV_SYMBOL_UP "  Display", 139, CONFIG_DISPLAY);
    status_back = nav(status_panel, LV_SYMBOL_UP "  Menu", 139, CONFIG_BACK);
    dial_label(status_panel, "Notifications", &font_muli_28, DIAL_PAPER, 0, -103);
    dial_label(status_panel, "BATTERY ESTIMATE", LV_FONT_DEFAULT, DIAL_MUTED, 0, -60);
    battery_label = dial_label(status_panel, "", &font_muli_28, DIAL_ACCENT, 0, -32);
    dial_mark(status_panel, 200, 2, 0, 0, DIAL_TRACK, 1);
    dial_label(status_panel, "BLUETOOTH", LV_FONT_DEFAULT, DIAL_MUTED, 0, 27);
    dial_label(status_panel, keyboard_ble_device_name(), &font_muli_28, DIAL_PAPER, 0, 57);
    connection_label = dial_label(status_panel, "", &font_muli_20, DIAL_BREAK, 0, 90);
    display_panel = dial_panel(root);
    watch_swipe(display_panel, LV_EVENT_GESTURE_DOWN);
    display_back = nav(display_panel, LV_SYMBOL_DOWN "  Settings", -132, CONFIG_OPEN);
    dial_label(display_panel, "Display", &font_muli_28, DIAL_PAPER, 0, -80);
    dial_label(display_panel, "IDLE BRIGHTNESS", LV_FONT_DEFAULT, DIAL_MUTED, 0, -33);
    dial_label(display_panel, "20%", &font_muli_28, DIAL_ACCENT, 0, -3);
    dial_label(display_panel, "After 5 minutes", &font_muli_20, DIAL_MUTED, 0, 37);
    light_test = dial_button(display_panel, 214, 48, 0, 97, DIAL_TILE, CONFIG_DIM_TEST, on_action);
    dial_label(light_test, "Preview for 3 s", &font_muli_20, DIAL_PAPER, 0, 0);
    lv_timer_create(light_tick, 150, NULL);
    lv_timer_create(refresh_status, 1000, NULL);
    battery_init();
    config_view_hide();
}
bool config_view_visible(void) { return visible; }

void config_view_swipe(bool up)
{
    lv_obj_t *page = home;
    if (lv_obj_is_visible(status_panel)) page = status_panel;
    else if (lv_obj_is_visible(panel)) page = panel;
    else if (lv_obj_is_visible(display_panel)) page = display_panel;
    if (lv_obj_is_visible(page))
        lv_obj_send_event(page, up ? LV_EVENT_GESTURE_UP : LV_EVENT_GESTURE_DOWN, NULL);
}

lv_obj_t *config_view_button(int id)
{
    if (id == CONFIG_OPEN) return lv_obj_is_visible(display_panel) ? display_back : open_settings;
    if (id == CONFIG_STATUS) return open_status;
    if (id == CONFIG_DISPLAY) return open_display;
    if (id == CONFIG_OFFICE) return toggle;
    if (id == CONFIG_DIM_TEST) return light_test;
    if (id == CONFIG_BACK && lv_obj_is_visible(status_panel)) return status_back;
    if (id == CONFIG_BACK && lv_obj_is_visible(display_panel)) return display_back;
    if (id == CONFIG_BACK) return back;
    if (id == CONFIG_PREVIEW) return preview;
    if (id == CONFIG_INTERVAL_PREV) return interval_prev;
    if (id == CONFIG_INTERVAL_NEXT) return interval_next;
    return NULL;
}
