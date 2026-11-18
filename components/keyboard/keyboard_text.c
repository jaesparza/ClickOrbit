#include "keyboard_text.h"
#include <stddef.h>

const char *keyboard_text_for_direction(int direction)
{
    if (!direction) return NULL;
    return direction > 0 ? "Hello" : "World";
}

bool keyboard_letter(char c, uint8_t *modifier, uint8_t *key)
{
    if (!modifier || !key) return false;
    bool upper = c >= 'A' && c <= 'Z';
    if (upper) c += 'a' - 'A';
    if (c < 'a' || c > 'z') return false;
    *modifier = upper ? 0x02 : 0; // HID left shift
    *key = (uint8_t)(0x04 + c - 'a'); // HID usage A..Z
    return true;
}
