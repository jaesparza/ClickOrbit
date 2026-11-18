#pragma once
// Pure HID mapping for the two demo words. No hardware, allocation, or retained input.
#include <stdbool.h>
#include <stdint.h>
const char *keyboard_text_for_direction(int direction);
bool keyboard_ascii(char c, uint8_t *modifier, uint8_t *key);
