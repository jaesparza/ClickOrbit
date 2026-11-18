#pragma once

#include <stdbool.h>
#include <stdint.h>

// BLE HID transport. Called only from the keyboard worker after keyboard_ble_init().
bool keyboard_ble_init(void);
bool keyboard_ble_connected(void);
const char *keyboard_ble_device_name(void);
bool keyboard_ble_send_keyboard(uint8_t modifier, const uint8_t keys[6]);
bool keyboard_ble_send_mouse(int8_t x, int8_t wheel);
