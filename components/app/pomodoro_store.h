#pragma once
#include <stdint.h>

// Load the total at startup. Queue an absolute value on completion or manual clear.
uint32_t pomo_store_init(void);
void pomo_store_save(uint32_t total);
