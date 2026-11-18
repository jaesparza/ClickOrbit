#include "jiggler_schedule.h"
bool jiggler_due(jiggler_schedule_t *s, bool enabled, unsigned generation, int64_t now)
{
    if (!enabled) { s->deadline = 0; return false; }
    if (!s->deadline || s->generation != generation) {
        s->generation = generation;
        s->deadline = now + JIGGLER_INTERVAL_US;
        return false;
    }
    if (now < s->deadline) return false;
    s->deadline = now + JIGGLER_INTERVAL_US;
    return true;
}
