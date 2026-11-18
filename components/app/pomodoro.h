#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum { POMO_FOCUS, POMO_BREAK, POMO_LONG_BREAK } pomo_phase_t;
typedef enum { POMO_READY, POMO_RUNNING, POMO_PAUSED } pomo_mode_t;
typedef struct {
    pomo_phase_t phase;
    pomo_mode_t mode;
    int minutes;
    int focus_minutes;
    unsigned round;  // Current focus/break pair, 1 through 4.
    uint32_t completed;
    int64_t left_ms;
    int64_t deadline_ms;
} pomodoro_t;

void pomo_init(pomodoro_t *p);
void pomo_adjust(pomodoro_t *p, int detent);
void pomo_toggle(pomodoro_t *p, int64_t now_ms);
void pomo_clear_completed(pomodoro_t *p);
void pomo_restart(pomodoro_t *p);
int64_t pomo_remaining(const pomodoro_t *p, int64_t now_ms);
// Finishes an expired phase once. The next phase waits for a tap.
bool pomo_finish(pomodoro_t *p, int64_t now_ms);
