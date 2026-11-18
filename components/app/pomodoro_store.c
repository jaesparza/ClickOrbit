#include "pomodoro_store.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"

static QueueHandle_t updates;
static nvs_handle_t storage;

static void writer(void *arg)
{
    (void)arg;
    uint32_t total;
    for (;;) {
        xQueueReceive(updates, &total, portMAX_DELAY);
        for (;;) {
            esp_err_t err = nvs_set_u32(storage, "completed", total);
            if (err == ESP_OK) err = nvs_commit(storage);
            if (err == ESP_OK) break;
            ESP_LOGW("pomodoro", "saving total: %s", esp_err_to_name(err));
            // Retry the newest value, including a reset queued after a completion.
            xQueueReceive(updates, &total, pdMS_TO_TICKS(5000));
        }
    }
}
uint32_t pomo_store_init(void)
{
    uint32_t total = 0;
    esp_err_t err = nvs_open("pomodoro", NVS_READWRITE, &storage);
    if (err == ESP_OK) {
        err = nvs_get_u32(storage, "completed", &total);
        if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
        if (err == ESP_OK) {
            updates = xQueueCreate(1, sizeof(total));
            if (updates && xTaskCreate(writer, "pomo_save", 3072, NULL, 2, NULL) == pdPASS)
                return total;
            if (updates) vQueueDelete(updates);
            updates = NULL;
            err = ESP_ERR_NO_MEM;
        }
        nvs_close(storage);
    }
    ESP_LOGE("pomodoro", "total storage unavailable: %s", esp_err_to_name(err));
    return total;
}
void pomo_store_save(uint32_t total)
{
    if (updates) xQueueOverwrite(updates, &total);
}
