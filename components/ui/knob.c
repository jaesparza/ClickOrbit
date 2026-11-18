// knob.c — rotary knob ESP driver: 3 ms GPIO poll → knob_decode.c → ordered detent queue →
// 30 ms LVGL timer that drains it into the user's callback. Pins and the direction sign come from
// board.h only. No interrupts: the switches bounce, and the decoder is built around sampling.
#include "knob.h"
#include "knob_decode.h"
#include "ui.h"
#include "board.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdatomic.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG = "knob";

#define POLL_US   3000   // 3 ms sample — the proven cadence for this switch-pair knob
#define PUMP_MS   30     // how often the LVGL task drains the accumulator

static knob_decode_state_t s_dec;
static QueueHandle_t       s_steps;    // preserves reversals for the unlock combination
static atomic_bool         s_overflow;
static void              (*s_on_step)(int dir);

// Runs on the esp_timer task every 3 ms. gpio_get_level() == 0 means "channel pulled LOW", which
// is the active detent pulse the decoder wants.
static void knob_poll_cb(void *arg)
{
    (void)arg;
    bool a_low = gpio_get_level(BOARD_KNOB_A) == 0;
    bool b_low = gpio_get_level(BOARD_KNOB_B) == 0;
    int step = knob_decode_step(&s_dec, a_low, b_low);
    if (step) {
        step *= BOARD_KNOB_DIRECTION;
        if (xQueueSend(s_steps, &step, 0) != pdTRUE) atomic_store(&s_overflow, true);
    }
}

// Runs on the LVGL task (lock held). One callback per detent keeps the user code trivial.
static void knob_pump_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_on_step) return;
    for (int n = 0; n < 64; ++n) {
        if (atomic_exchange(&s_overflow, false)) {
            xQueueReset(s_steps);
            s_on_step(0); // invalidate partial combinations after any lost input
            ESP_LOGW(TAG, "detent queue overflow; input sequence reset");
            return;
        }
        int direction;
        if (xQueueReceive(s_steps, &direction, 0) != pdTRUE) return;
        s_on_step(direction);
    }
}

static void knob_gpio_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BOARD_KNOB_A) | (1ULL << BOARD_KNOB_B),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,     // idle HIGH; a detent momentarily pulls one low
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,      // polled, not interrupt-driven (bounce-tolerant FSM)
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
}

void knob_init(void (*on_step)(int dir))
{
    s_on_step = on_step;
    s_steps = xQueueCreate(64, sizeof(int));
    ESP_ERROR_CHECK(s_steps ? ESP_OK : ESP_ERR_NO_MEM);
    knob_gpio_init();

    const esp_timer_create_args_t targs = {
        .callback = knob_poll_cb, .name = "knob_poll", .dispatch_method = ESP_TIMER_TASK,
    };
    static esp_timer_handle_t s_poll_timer;   // runs for the device's lifetime
    ESP_ERROR_CHECK(esp_timer_create(&targs, &s_poll_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_poll_timer, POLL_US));

    if (on_step) {
        if (ui_lock(100)) {
            lv_timer_create(knob_pump_cb, PUMP_MS, NULL);
            ui_unlock();
        } else {
            ESP_LOGE(TAG, "no LVGL lock — knob callback disabled");
        }
    }
    ESP_LOGI(TAG, "knob up (poll %d us, A=GPIO%d B=GPIO%d, non-quadrature)",
             POLL_US, BOARD_KNOB_A, BOARD_KNOB_B);
}

int knob_take_steps(void)
{
    int total = 0, direction;
    if (!s_steps) return 0;
    while (xQueueReceive(s_steps, &direction, 0) == pdTRUE) total += direction;
    return total;
}
