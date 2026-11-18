// Composite CDC + boot keyboard. Descriptor storage must live for the USB lifetime.
#include "tinyusb.h"
#include "keyboard_usb.h"

static const uint8_t s_report[] = { TUD_HID_REPORT_DESC_KEYBOARD() };
static const uint8_t s_mouse_report[] = { TUD_HID_REPORT_DESC_MOUSE() };
static const char *s_strings[] = {
    (char[]){0x09, 0x04}, "My Knob", "K518 Keyboard", "my-knob-001",
    "Knob console", "Knob keyboard", "Knob mouse",
};
static const uint8_t s_config[] = {
    TUD_CONFIG_DESCRIPTOR(1, 4, 0,
        TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + 2 * TUD_HID_DESC_LEN, 0, 500),
    TUD_CDC_DESCRIPTOR(0, 4, 0x81, 8, 0x02, 0x82, 64),
    TUD_HID_DESCRIPTOR(2, 5, HID_ITF_PROTOCOL_KEYBOARD, sizeof(s_report), 0x83, 8, 10),
    TUD_HID_DESCRIPTOR(3, 6, HID_ITF_PROTOCOL_MOUSE, sizeof(s_mouse_report), 0x84, 8, 10),
};

void keyboard_usb_install(void)
{
    const tinyusb_config_t config = {
        .string_descriptor = s_strings,
        .string_descriptor_count = sizeof(s_strings) / sizeof(s_strings[0]),
        .configuration_descriptor = s_config,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&config));
}

const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance)
{
    return instance == 0 ? s_report : s_mouse_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
    hid_report_type_t type, uint8_t *buffer, uint16_t length)
{
    (void)instance; (void)report_id; (void)type; (void)buffer; (void)length;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
    hid_report_type_t type, const uint8_t *buffer, uint16_t length)
{
    if (instance == 0 && report_id == 0 && type == HID_REPORT_TYPE_OUTPUT && length >= 1) {
        keyboard_set_leds(buffer[0]);
    }
}
