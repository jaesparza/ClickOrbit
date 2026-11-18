#pragma once
void battery_init(void);
// Estimated percentage, or -1 if the ADC reading is unavailable.
int battery_estimate(void);
