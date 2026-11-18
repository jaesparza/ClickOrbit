#pragma once
#include <stdbool.h>
#include <stdint.h>
#define JIGGLER_DEFAULT_INTERVAL_MS UINT32_C(300000)
#define JIGGLER_TEST_INTERVAL_MS UINT32_C(2000)
// Pure monotonic scheduling. A generation change restarts the selected interval.
typedef struct { unsigned generation; int64_t deadline; } jiggler_schedule_t;
bool jiggler_due(jiggler_schedule_t *schedule, bool enabled, unsigned generation,
                 uint32_t interval_ms, int64_t now);
