#include "jiggler_schedule.h"
bool jiggler_due(jiggler_schedule_t *s, bool enabled, unsigned generation,
                 uint32_t interval_ms, int64_t now)
{
    if (!enabled) { s->deadline = 0; return false; }
    if (!interval_ms) interval_ms = JIGGLER_DEFAULT_INTERVAL_MS;
    if (!s->deadline || s->generation != generation) {
        s->generation = generation;
        s->deadline = now + (int64_t)interval_ms * 1000;
        return false;
    }
    if (now < s->deadline) return false;
    s->deadline = now + (int64_t)interval_ms * 1000;
    return true;
}
