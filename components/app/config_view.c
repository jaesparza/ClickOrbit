#include "config_view.h"
#include "dial_style.h"
#include "haptic.h"
#include "pomodoro_store.h"
#include "keyboard.h"
#include "keyboard_ble.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "board_jc3636k518c.h"

static lv_obj_t *panel, *home, *back, *toggle, *state, *preview;
static lv_obj_t *interval_prev, *interval_next, *interval_label;
static bool visible;
static lv_obj_t *status_panel, *battery_label, *device_label, *connection_label;
static lv_timer_t *status_timer;
static adc_oneshot_unit_handle_t battery_adc;
static adc_cali_handle_t battery_calibration;
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
    lv_color_t color = selected_interval() == 4 ? lv_color_hex(DIAL_ORANGE) : lv_color_hex(DIAL_PAPER);
    lv_obj_set_style_text_color(interval_label, color, 0);
}

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

static void refresh_status(lv_timer_t *timer)
{
    (void)timer;
    if (!status_panel || !lv_obj_is_visible(status_panel)) return;
    int raw = 0, pin_mv = 0;
    if (battery_adc && adc_oneshot_read(battery_adc, ADC_CHANNEL_0, &raw) == ESP_OK &&
        battery_calibration && adc_cali_raw_to_voltage(battery_calibration, raw, &pin_mv) == ESP_OK) {
        int battery_mv = pin_mv * 2;
        int percent = (battery_mv - 3300) * 100 / 900;
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        lv_label_set_text_fmt(battery_label, "Battery  %d%%", percent);
    } else {
        lv_label_set_text(battery_label, "Battery unavailable");
    }
    lv_label_set_text_fmt(device_label, "Bluetooth: %s", keyboard_ble_device_name());
    bool connected = keyboard_ble_connected();
    lv_label_set_text(connection_label, connected ? "Connected" : "Disconnected");
    lv_obj_set_style_text_color(connection_label, lv_color_hex(connected ? DIAL_BREAK : DIAL_ORANGE), 0);
}

void config_view_notify(bool open)
{
    visible = open;
    lv_obj_set_hidden(home, open);
    lv_obj_set_hidden(status_panel, !open);
    if (open) refresh_status(NULL);
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
    lv_obj_t *target = lv_event_get_target(event);
    lv_event_code_t code = lv_event_get_code(event);
    if (target == home) {
        if (code == LV_EVENT_GESTURE_UP) navigate(true);
        else config_view_notify(true);
    } else if (target == status_panel && code == LV_EVENT_GESTURE_UP) {
        config_view_notify(false);
    } else if (target == panel && code == LV_EVENT_GESTURE_DOWN) {
        navigate(false);
    }
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
    } else if (id == CONFIG_INTERVAL_PREV || id == CONFIG_INTERVAL_NEXT) {
        unsigned count = sizeof(intervals_ms) / sizeof(intervals_ms[0]);
        unsigned selected = selected_interval();
        selected = id == CONFIG_INTERVAL_NEXT ? (selected + 1) % count : (selected + count - 1) % count;
        pomo_store_set_jiggler_interval_ms(intervals_ms[selected]);
        keyboard_set_jiggler_interval_ms(intervals_ms[selected]);
        display_interval();
    }
}

void config_view_build(lv_obj_t *root, lv_obj_t *home_page)
{
    home = home_page;
    watch_swipe(home, LV_EVENT_GESTURE_UP);
    watch_swipe(home, LV_EVENT_GESTURE_DOWN);
    dial_label(home, LV_SYMBOL_UP "  Settings", LV_FONT_DEFAULT, DIAL_MUTED, 0, 136);
    dial_label(home, LV_SYMBOL_DOWN "  Notifications", LV_FONT_DEFAULT, DIAL_MUTED, 0, 158);
    panel = dial_panel(root);
    watch_swipe(panel, LV_EVENT_GESTURE_DOWN);
    back = dial_button(panel, 110, 44, 0, -126, DIAL_TILE, CONFIG_BACK, on_action);
    dial_label(back, LV_SYMBOL_DOWN "  Menu", LV_FONT_DEFAULT, DIAL_PAPER, 0, 0);
    dial_label(panel, "Configuration", &font_muli_28, DIAL_PAPER, 0, -78);
    toggle = dial_button(panel, 246, 50, 0, -29, DIAL_TILE, CONFIG_OFFICE, on_action);
    dial_label(toggle, "Office mode", &font_muli_20, DIAL_PAPER, -36, 0);
    state = dial_label(toggle, "", &font_muli_20, DIAL_MUTED, 82, 0);
    dial_label(panel, "Jiggler interval", &font_muli_20, DIAL_MUTED, 0, 16);
    interval_prev = dial_button(panel, 48, 40, -91, 59, DIAL_TILE, CONFIG_INTERVAL_PREV, on_action);
    dial_label(interval_prev, LV_SYMBOL_LEFT, LV_FONT_DEFAULT, DIAL_PAPER, 0, 0);
    interval_label = dial_label(panel, "", &font_muli_20, DIAL_PAPER, 0, 59);
    interval_next = dial_button(panel, 48, 40, 91, 59, DIAL_TILE, CONFIG_INTERVAL_NEXT, on_action);
    dial_label(interval_next, LV_SYMBOL_RIGHT, LV_FONT_DEFAULT, DIAL_PAPER, 0, 0);
    preview = dial_button(panel, 166, 40, 0, 119, DIAL_TILE, CONFIG_PREVIEW, on_action);
    dial_label(preview, "Test alert", &font_muli_20, DIAL_PAPER, 0, 0);
    haptic_set_office_mode(pomo_store_office());
    keyboard_set_jiggler_interval_ms(pomo_store_jiggler_interval_ms());
    display_mode();
    display_interval();
    status_panel = dial_panel(root);
    watch_swipe(status_panel, LV_EVENT_GESTURE_UP);
    watch_swipe(status_panel, LV_EVENT_GESTURE_DOWN);
    dial_label(status_panel, "Notifications", &font_muli_28, DIAL_PAPER, 0, -90);
    battery_label = dial_label(status_panel, "Battery level unavailable", &font_muli_20, DIAL_PAPER, 0, -34);
    device_label = dial_label(status_panel, "", &font_muli_20, DIAL_MUTED, 0, 20);
    connection_label = dial_label(status_panel, "", &font_muli_20, DIAL_PAPER, 0, 58);
    lv_obj_set_hidden(status_panel, true);
    status_timer = lv_timer_create(refresh_status, 1000, NULL);
    (void)status_timer;
    adc_oneshot_unit_init_cfg_t adc_config = {.unit_id = ADC_UNIT_1};
    if (adc_oneshot_new_unit(&adc_config, &battery_adc) == ESP_OK) {
        adc_oneshot_chan_cfg_t channel_config = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT};
        adc_oneshot_config_channel(battery_adc, ADC_CHANNEL_0, &channel_config);
        adc_cali_curve_fitting_config_t calibration_config = {
            .unit_id = ADC_UNIT_1, .chan = ADC_CHANNEL_0, .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        adc_cali_create_scheme_curve_fitting(&calibration_config, &battery_calibration);
    }
    config_view_hide();
}
bool config_view_visible(void) { return visible; }

void config_view_swipe(bool up)
{
    lv_obj_t *page = lv_obj_is_visible(status_panel) ? status_panel : (up ? home : panel);
    if (lv_obj_is_visible(page))
        lv_obj_send_event(page, up ? LV_EVENT_GESTURE_UP : LV_EVENT_GESTURE_DOWN, NULL);
}

lv_obj_t *config_view_button(int id)
{
    if (id == CONFIG_OFFICE) return toggle;
    if (id == CONFIG_BACK) return back;
    if (id == CONFIG_PREVIEW) return preview;
    if (id == CONFIG_INTERVAL_PREV) return interval_prev;
    if (id == CONFIG_INTERVAL_NEXT) return interval_next;
    return NULL;
}
