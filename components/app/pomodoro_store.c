#include "pomodoro_store.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"

static QueueHandle_t updates;
static nvs_handle_t storage;
typedef struct { uint32_t completed; uint8_t office; } preferences_t;
static preferences_t latest; // UI-owned. The worker receives a copy.

static void writer(void *arg)
{
    (void)arg;
    preferences_t snapshot;
    for (;;) {
        xQueueReceive(updates, &snapshot, portMAX_DELAY);
        for (;;) {
            esp_err_t err = nvs_set_u32(storage, "completed", snapshot.completed);
            if (err == ESP_OK) err = nvs_set_u8(storage, "office", snapshot.office);
            if (err == ESP_OK) err = nvs_commit(storage);
            if (err == ESP_OK) break;
            ESP_LOGW("pomodoro", "saving preferences: %s", esp_err_to_name(err));
            // Retry the newest value, including a reset queued after a completion.
            xQueueReceive(updates, &snapshot, pdMS_TO_TICKS(5000));
        }
    }
}
uint32_t pomo_store_init(void)
{
    esp_err_t err = nvs_open("pomodoro", NVS_READWRITE, &storage);
    if (err == ESP_OK) {
        err = nvs_get_u32(storage, "completed", &latest.completed);
        if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
        if (err == ESP_OK) err = nvs_get_u8(storage, "office", &latest.office);
        if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
        if (err == ESP_OK) {
            updates = xQueueCreate(1, sizeof(latest));
            if (updates && xTaskCreate(writer, "pomo_save", 3072, NULL, 2, NULL) == pdPASS)
                return latest.completed;
            if (updates) vQueueDelete(updates);
            updates = NULL;
            err = ESP_ERR_NO_MEM;
        }
        nvs_close(storage);
    }
    ESP_LOGE("pomodoro", "preference storage unavailable: %s", esp_err_to_name(err));
    return latest.completed;
}
void pomo_store_save(uint32_t total)
{
    latest.completed = total;
    if (updates) xQueueOverwrite(updates, &latest);
}
bool pomo_store_office(void) { return latest.office != 0; }
void pomo_store_set_office(bool enabled)
{
    latest.office = enabled;
    if (updates) xQueueOverwrite(updates, &latest);
}
