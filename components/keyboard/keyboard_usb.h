#pragma once
#include <stdint.h>
// Private USB integration: descriptors/install and host LED updates from TinyUSB task.
void keyboard_usb_install(void);
void keyboard_set_leds(uint8_t leds);
