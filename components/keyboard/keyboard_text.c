#include "keyboard_text.h"
#include <stddef.h>

const char *keyboard_text_for_direction(int direction)
{
    if (!direction) return NULL;
    return direction > 0 ? "Hello" : "World";
}

bool keyboard_ascii(char c, uint8_t *modifier, uint8_t *key)
{
    if (!modifier || !key) return false;
    *modifier = 0;
    if (c >= 'a' && c <= 'z') *key = (uint8_t)(0x04 + c - 'a');
    else if (c >= 'A' && c <= 'Z') {
        *modifier = 0x02;
        *key = (uint8_t)(0x04 + c - 'A');
    } else if (c >= '1' && c <= '9') *key = (uint8_t)(0x1E + c - '1');
    else if (c == '0') *key = 0x27;
    else if (c >= '!' && c <= ')') {
        static const uint8_t shifted_digit_keys[] = {0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27};
        *modifier = 0x02;
        *key = shifted_digit_keys[c - '!'];
    } else {
        switch (c) {
        case ' ': *key = 0x2C; break;
        case '\n': *key = 0x28; break;
        case '\t': *key = 0x2B; break;
        case '@': *modifier = 0x02; *key = 0x1F; break;
        case '^': *modifier = 0x02; *key = 0x23; break;
        case '*': *modifier = 0x02; *key = 0x25; break;
        case '-': *key = 0x2D; break;
        case '_': *modifier = 0x02; *key = 0x2D; break;
        case '=': *key = 0x2E; break;
        case '+': *modifier = 0x02; *key = 0x2E; break;
        case '[': *key = 0x2F; break;
        case '{': *modifier = 0x02; *key = 0x2F; break;
        case ']': *key = 0x30; break;
        case '}': *modifier = 0x02; *key = 0x30; break;
        case '\\': *key = 0x31; break;
        case '|': *modifier = 0x02; *key = 0x31; break;
        case ';': *key = 0x33; break;
        case ':': *modifier = 0x02; *key = 0x33; break;
        case '\'': *key = 0x34; break;
        case '"': *modifier = 0x02; *key = 0x34; break;
        case '`': *key = 0x35; break;
        case '~': *modifier = 0x02; *key = 0x35; break;
        case ',': *key = 0x36; break;
        case '<': *modifier = 0x02; *key = 0x36; break;
        case '.': *key = 0x37; break;
        case '>': *modifier = 0x02; *key = 0x37; break;
        case '/': *key = 0x38; break;
        case '?': *modifier = 0x02; *key = 0x38; break;
        default: return false;
        }
    }
    return true;
}
