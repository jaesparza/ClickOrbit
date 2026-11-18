#include "pomodoro.h"

static void prepare(pomodoro_t *p, int minutes)
{
    p->mode = POMO_READY;
    p->minutes = minutes;
    p->left_ms = minutes * 60000;
}
void pomo_init(pomodoro_t *p)
{
    *p = (pomodoro_t){.focus_minutes = 25, .round = 1};
    prepare(p, p->focus_minutes);
}
void pomo_adjust(pomodoro_t *p, int detent)
{
    int minutes = p->minutes + detent;
    if (p->mode != POMO_READY || minutes < 1 || minutes > 60) return;
    prepare(p, minutes);
    if (p->phase == POMO_FOCUS) p->focus_minutes = minutes;
}
int64_t pomo_remaining(const pomodoro_t *p, int64_t now_ms)
{
    if (p->mode != POMO_RUNNING) return p->left_ms;
    return now_ms < p->deadline_ms ? p->deadline_ms - now_ms : 0;
}
static void next_focus(pomodoro_t *p)
{
    p->round = p->round % 4 + 1;
    p->phase = POMO_FOCUS;
    prepare(p, p->focus_minutes);
}
void pomo_clear_completed(pomodoro_t *p)
{
    p->completed = 0;
}
bool pomo_finish(pomodoro_t *p, int64_t now_ms)
{
    if (p->mode != POMO_RUNNING || pomo_remaining(p, now_ms)) return false;
    if (p->phase == POMO_FOCUS) {
        if (p->completed < UINT32_MAX) p->completed++;
        p->phase = p->round == 4 ? POMO_LONG_BREAK : POMO_BREAK;
        prepare(p, p->phase == POMO_LONG_BREAK ? 15 : 5);
    } else {
        next_focus(p);
    }
    return true;
}
void pomo_toggle(pomodoro_t *p, int64_t now_ms)
{
    if (pomo_finish(p, now_ms)) return;
    if (p->mode == POMO_RUNNING) {
        p->left_ms = pomo_remaining(p, now_ms);
        p->mode = POMO_PAUSED;
    } else {
        p->deadline_ms = now_ms + p->left_ms;
        p->mode = POMO_RUNNING;
    }
}
void pomo_restart(pomodoro_t *p)
{
    if (p->phase != POMO_FOCUS) next_focus(p);
    else if (p->mode == POMO_PAUSED) prepare(p, p->focus_minutes);
}
