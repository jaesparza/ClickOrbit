#pragma once
// app owns Password/Jiggler/Password 2 touch actions and ring scrolling.
// Depends on ui, haptic and keyboard.
// LVGL functions/test hooks require ui_lock; USB I/O runs on its own worker.
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    APP_IDLE = 0,      // teal, breathing
    APP_ACTIVE,        // red, solid
    APP_UPLOADING,     // blue, progress arc
    APP_DONE,          // green
    APP_ERROR,         // red blink
} app_state_t;

// Build the screen, wire touch actions, and start knob polling for mouse wheel output.
void app_init(void);
// Read while holding ui_lock, as with other app hooks.
bool app_is_unlocked(void);

// Test hooks (components/testctl): run the SAME handlers hardware input runs. Caller holds
// ui_lock(). Add one per new input; the console never reaches into this component's statics.
void app_inject_knob(int steps);                   // real ring handler, one wheel tick per detent
void app_inject_tap(void);                         // background tap: no-op
void app_inject_hold(void);                        // background hold: no-op
void app_inject_button(int number);               // real short-click on button 1, 2 or 3
void app_inject_reset(void);                       // Jiggler off, last password reset
void app_state_line(char *buf, size_t cap);        // "last=N jiggler=on/off moves=N"
