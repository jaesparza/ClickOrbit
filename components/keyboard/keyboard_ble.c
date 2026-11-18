#include "keyboard_ble.h"

#include <stdatomic.h>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_hid_common.h"
#include "esp_hidd.h"
#include "esp_hidd_gatts.h"
#include "esp_log.h"

static const char *TAG = "keyboard_ble";
#define BLE_DEVICE_NAME "myAssistant"
static atomic_bool s_connected;
static atomic_uint s_advertising_data_ready;
static esp_hidd_dev_t *s_device;
static esp_ble_adv_params_t s_adv = {
    .adv_int_min = 0x20, .adv_int_max = 0x40, .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC, .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// Two report IDs let Windows/macOS/Linux expose one BLE peripheral as both HID classes.
static const uint8_t s_report_map[] = {
    0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x85, 0x01,
    0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00,
    0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
    0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x65,
    0x05, 0x07, 0x19, 0x00, 0x29, 0x65, 0x81, 0x00,
    0xC0,
    0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x85, 0x02,
    0x09, 0x01, 0xA1, 0x00, 0x05, 0x09, 0x19, 0x01,
    0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 0x95, 0x03,
    0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05,
    0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31,
    0x09, 0x38, 0x15, 0x81, 0x25, 0x7F, 0x75, 0x08,
    0x95, 0x03, 0x81, 0x06, 0xC0, 0xC0,
};
static esp_hid_raw_report_map_t s_maps[] = {
    {.data = s_report_map, .len = sizeof(s_report_map)},
};
static const esp_hid_device_config_t s_config = {
    .vendor_id = 0x303A,
    .product_id = 0x1001,
    .version = 0x0100,
    .device_name = "myAssistant",
    .manufacturer_name = "myAssistant",
    .serial_number = "K518",
    .report_maps = s_maps,
    .report_maps_len = 1,
};

static void gap_event(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    if (event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT || event == ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT) {
        esp_err_t status = event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT ?
                           param->adv_data_cmpl.status : param->scan_rsp_data_cmpl.status;
        if (status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "BLE advertising data setup failed: %d", status);
            return;
        }
        unsigned ready_bit = event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT ? 1U : 2U;
        if ((atomic_fetch_or(&s_advertising_data_ready, ready_bit) | ready_bit) == 3U) {
            esp_err_t err = esp_ble_gap_start_advertising(&s_adv);
            if (err != ESP_OK) ESP_LOGE(TAG, "advertising start failed: %s", esp_err_to_name(err));
        }
    } else if (event == ESP_GAP_BLE_ADV_START_COMPLETE_EVT) {
        if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) ESP_LOGI(TAG, "advertising as myAssistant");
        else ESP_LOGE(TAG, "advertising failed: %d", param->adv_start_cmpl.status);
    } else if (event == ESP_GAP_BLE_SEC_REQ_EVT) {
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
    } else if (event == ESP_GAP_BLE_AUTH_CMPL_EVT) {
        ESP_LOGI(TAG, "BLE pairing %s", param->ble_security.auth_cmpl.success ? "complete" : "failed");
    }
}

static void hid_event(void *handler_arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)handler_arg;
    (void)base;
    (void)data;
    if (id == ESP_HIDD_CONNECT_EVENT) {
        atomic_store(&s_connected, true);
        ESP_LOGI(TAG, "PC connected over BLE HID");
    } else if (id == ESP_HIDD_DISCONNECT_EVENT) {
        atomic_store(&s_connected, false);
        ESP_LOGI(TAG, "BLE HID disconnected");
        esp_err_t err = esp_ble_gap_start_advertising(&s_adv);
        if (err != ESP_OK) ESP_LOGW(TAG, "advertising restart failed: %s", esp_err_to_name(err));
    }
}

bool keyboard_ble_init(void)
{
    esp_err_t err = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) goto fail;
    esp_bt_controller_config_t bt_config = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((err = esp_bt_controller_init(&bt_config)) != ESP_OK) goto fail;
    if ((err = esp_bt_controller_enable(ESP_BT_MODE_BLE)) != ESP_OK) goto fail;
    if ((err = esp_bluedroid_init()) != ESP_OK) goto fail;
    if ((err = esp_bluedroid_enable()) != ESP_OK) goto fail;
    if ((err = esp_ble_gap_register_callback(gap_event)) != ESP_OK) goto fail;
    if ((err = esp_ble_gatts_register_callback(esp_hidd_gatts_event_handler)) != ESP_OK) goto fail;
    if ((err = esp_ble_gap_set_device_name("myAssistant")) != ESP_OK) goto fail;

    esp_ble_auth_req_t auth = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t io_cap = ESP_IO_CAP_NONE;
    uint8_t key_mask = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t key_size = 16;
    if ((err = esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth, sizeof(auth))) != ESP_OK) goto fail;
    if ((err = esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &io_cap, sizeof(io_cap))) != ESP_OK) goto fail;
    if ((err = esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &key_mask, sizeof(key_mask))) != ESP_OK) goto fail;
    if ((err = esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &key_mask, sizeof(key_mask))) != ESP_OK) goto fail;
    if ((err = esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size))) != ESP_OK) goto fail;

    const uint8_t hid_service_uuid[] = {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0x12,0x18,0x00,0x00};
    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false, .include_name = false, .include_txpower = false,
        .appearance = ESP_HID_APPEARANCE_KEYBOARD,
        .service_uuid_len = sizeof(hid_service_uuid),
        .p_service_uuid = (uint8_t *)hid_service_uuid,
        .flag = ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT,
    };
    esp_ble_adv_data_t scan_rsp = {
        .set_scan_rsp = true, .include_name = true, .include_txpower = true,
    };
    if ((err = esp_ble_gap_config_adv_data(&adv_data)) != ESP_OK) goto fail;
    if ((err = esp_ble_gap_config_adv_data(&scan_rsp)) != ESP_OK) goto fail;
    if ((err = esp_hidd_dev_init(&s_config, ESP_HID_TRANSPORT_BLE, hid_event, &s_device)) != ESP_OK) goto fail;
    ESP_LOGI(TAG, "BLE HID keyboard and mouse ready");
    return true;
fail:
    ESP_LOGE(TAG, "BLE HID initialization failed: %s", esp_err_to_name(err));
    return false;
}

bool keyboard_ble_connected(void)
{
    return atomic_load(&s_connected);
}

const char *keyboard_ble_device_name(void)
{
    return BLE_DEVICE_NAME;
}

bool keyboard_ble_send_keyboard(uint8_t modifier, const uint8_t keys[6])
{
    uint8_t report[8] = {modifier, 0};
    if (!keyboard_ble_connected() || !s_device) return false;
    if (keys) for (unsigned i = 0; i < 6; ++i) report[i + 2] = keys[i];
    return esp_hidd_dev_input_set(s_device, 0, 1, report, sizeof(report)) == ESP_OK;
}

bool keyboard_ble_send_mouse(int8_t x, int8_t wheel)
{
    uint8_t report[4] = {0, (uint8_t)x, 0, (uint8_t)wheel};
    if (!keyboard_ble_connected() || !s_device) return false;
    return esp_hidd_dev_input_set(s_device, 0, 2, report, sizeof(report)) == ESP_OK;
}
