#pragma once
#include <stdint.h>
#include <stdbool.h>

// Load the total at startup. Queue an absolute value on completion or manual clear.
uint32_t pomo_store_init(void);
void pomo_store_save(uint32_t total);
bool pomo_store_office(void);
void pomo_store_set_office(bool enabled);
uint32_t pomo_store_jiggler_interval_ms(void);
void pomo_store_set_jiggler_interval_ms(uint32_t interval_ms);
