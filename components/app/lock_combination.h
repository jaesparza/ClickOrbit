#pragma once
// Pure, task-local encoder combination. Zero initialization means locked.
#include <stdbool.h>
#include <stdint.h>
typedef struct { uint8_t progress; } lock_combination_t;
// Feed one signed detent. Incorrect direction clears progress, without reusing that detent.
// Returns true only once the exact five-left/five-right sequence has completed.
bool lock_combination_step(lock_combination_t *state, int direction);
