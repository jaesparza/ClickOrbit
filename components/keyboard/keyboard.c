#include "keyboard.h"
#include "keyboard_text.h"
#include "keyboard_usb.h"
#include "keyboard_ble.h"
#include "jiggler_schedule.h"
#include "esp_timer.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdatomic.h>

static const char *TAG = "keyboard";
static QueueHandle_t s_queue;
static atomic_bool s_caps_lock;
static atomic_bool s_jiggler;
static atomic_uint s_generation;
static atomic_uint s_jiggler_interval_ms = JIGGLER_DEFAULT_INTERVAL_MS;
static atomic_uint s_mouse_pairs;
static atomic_int s_wheel_total;

void keyboard_set_jiggler(bool enabled)
{
    atomic_store(&s_jiggler, enabled);
    atomic_fetch_add(&s_generation, 1);
}

bool keyboard_jiggler_enabled(void) { return atomic_load(&s_jiggler); }
void keyboard_set_jiggler_interval_ms(uint32_t interval_ms)
{
    if (interval_ms < JIGGLER_TEST_INTERVAL_MS) return;
    atomic_store(&s_jiggler_interval_ms, interval_ms);
    atomic_fetch_add(&s_generation, 1);
}
uint32_t keyboard_jiggler_interval_ms(void) { return atomic_load(&s_jiggler_interval_ms); }
unsigned keyboard_mouse_pairs(void) { return atomic_load(&s_mouse_pairs); }
int keyboard_wheel_total(void) { return atomic_load(&s_wheel_total); }

static bool mouse_report(int8_t x, int8_t wheel)
{
    if (keyboard_ble_connected()) {
        bool sent = keyboard_ble_send_mouse(x, wheel);
        if (sent) vTaskDelay(pdMS_TO_TICKS(20));
        return sent;
    }
    for (int i = 0; i < 100; ++i) {
        if (!tud_mounted() || tud_suspended()) return false;
        if (tud_hid_n_ready(1) && tud_hid_n_mouse_report(1, 0, 0, x, 0, wheel, 0)) {
            vTaskDelay(pdMS_TO_TICKS(20));
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return false;
}

static void jiggle(void)
{
    if (mouse_report(1, 0) && mouse_report(-1, 0)) atomic_fetch_add(&s_mouse_pairs, 1);
    else ESP_LOGW(TAG, "mouse movement failed: no active HID host");
}

bool keyboard_test_jiggle(void)
{
    int event = 0;
    return s_queue && xQueueSend(s_queue, &event, 0) == pdTRUE;
}

void keyboard_set_leds(uint8_t leds)
{
    atomic_store(&s_caps_lock, (leds & KEYBOARD_LED_CAPSLOCK) != 0);
}

// Bounded waits run only on the worker, never on the LVGL task.
static bool send_report(uint8_t modifier, const uint8_t *keys)
{
    if (keyboard_ble_connected()) {
        bool sent = keyboard_ble_send_keyboard(modifier, keys);
        if (sent) vTaskDelay(pdMS_TO_TICKS(20));
        return sent;
    }
    for (int i = 0; i < 100; ++i) {
        if (!tud_mounted()) return false;
        if (tud_hid_ready() && tud_hid_keyboard_report(0, modifier, keys)) {
            vTaskDelay(pdMS_TO_TICKS(20));
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return false;
}

static bool type_text(const char *text)
{
    for (; *text; ++text) {
        uint8_t modifier, key;
        uint8_t keys[6] = {0};
        if (!keyboard_ascii(*text, &modifier, &key)) return false;
        keys[0] = key;
        if (atomic_load(&s_caps_lock)) modifier ^= KEYBOARD_MODIFIER_LEFTSHIFT;
        if (!send_report(modifier, keys)) {
            send_report(0, NULL); // best effort release after a failed press
            return false;
        }
        if (!send_report(0, NULL)) return false;
    }
    return true;
}

static void keyboard_task(void *arg)
{
    (void)arg;
    int direction;
    jiggler_schedule_t schedule = {0};
    for (;;) {
        if (jiggler_due(&schedule, keyboard_jiggler_enabled(),
                        atomic_load(&s_generation), keyboard_jiggler_interval_ms(),
                        esp_timer_get_time()) &&
            keyboard_jiggler_enabled()) jiggle();
        if (xQueueReceive(s_queue, &direction, pdMS_TO_TICKS(100)) != pdTRUE) continue;
        if (direction == 0) { jiggle(); continue; }
        if (direction == 2 || direction == -2) {
            int wheel = direction > 0 ? -1 : 1; // clockwise scrolls down
            if (mouse_report(0, wheel)) atomic_fetch_add(&s_wheel_total, wheel);
            else ESP_LOGW(TAG, "mouse wheel report failed");
            continue;
        }
        if (!type_text(keyboard_text_for_direction(direction))) {
            ESP_LOGW(TAG, "USB transfer failed; discarding pending detents");
            xQueueReset(s_queue);
        }
        ESP_LOGI(TAG, "worker stack free=%u", (unsigned)uxTaskGetStackHighWaterMark(NULL));
    }
}

void keyboard_init(void)
{
    s_queue = xQueueCreate(16, sizeof(int));
    assert(s_queue);
    keyboard_usb_install();
    const tinyusb_config_cdcacm_t cdc = {
        .usb_dev = TINYUSB_USBDEV_0, .cdc_port = TINYUSB_CDC_ACM_0,
    };
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&cdc));
    ESP_ERROR_CHECK(esp_tusb_init_console(TINYUSB_CDC_ACM_0));
    if (!keyboard_ble_init()) ESP_LOGW(TAG, "BLE HID unavailable; USB HID remains active");
    BaseType_t started = xTaskCreate(keyboard_task, "keyboard", 4096, NULL, 3, NULL);
    ESP_ERROR_CHECK(started == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_LOGI(TAG, "USB and BLE keyboard + mouse ready");
}

bool keyboard_queue_direction(int direction)
{
    if (!s_queue || !direction || (!keyboard_ble_connected() && (!tud_mounted() || tud_suspended()))) return false;
    int event = direction > 0 ? 1 : -1;
    return xQueueSend(s_queue, &event, 0) == pdTRUE;
}

bool keyboard_queue_scroll(int direction)
{
    if (!s_queue || !direction || (!keyboard_ble_connected() && (!tud_mounted() || tud_suspended()))) return false;
    int event = direction > 0 ? 2 : -2;
    return xQueueSend(s_queue, &event, 0) == pdTRUE;
}
