#include "lock_combination.h"
bool lock_combination_step(lock_combination_t *s, int direction)
{
    if (s->progress == 10) return true;
    if (!direction) return false;
    int expected = s->progress < 5 ? -1 : 1;
    if ((direction > 0 ? 1 : -1) != expected) {
        s->progress = 0;
        return false;
    }
    return ++s->progress == 10;
}
