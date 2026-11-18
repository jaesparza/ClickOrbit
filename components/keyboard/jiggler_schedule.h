#pragma once
#include <stdbool.h>
#include <stdint.h>
#define JIGGLER_INTERVAL_US INT64_C(300000000)
// Pure monotonic scheduling. A generation change restarts the five-minute interval.
typedef struct { unsigned generation; int64_t deadline; } jiggler_schedule_t;
bool jiggler_due(jiggler_schedule_t *schedule, bool enabled, unsigned generation, int64_t now);
