#pragma once
// Owns USB HID keyboard + CDC console. Depends on TinyUSB/FreeRTOS, never LVGL.
// Initialize once before app/testctl. Queue calls are nonblocking from any task.
// Only direction values are queued; no caller-owned pointers are retained.
#include <stdbool.h>

void keyboard_init(void);
// One positive detent types Hello; one negative detent types World.
// Returns false if USB is unavailable or the bounded queue is full.
bool keyboard_queue_direction(int direction);
// One wheel tick per detent: positive/clockwise scrolls down, negative scrolls up.
bool keyboard_queue_scroll(int direction);
int keyboard_wheel_total(void); // signed count of successfully submitted wheel ticks
// Jiggler starts disabled. Enable schedules its first movement five minutes later.
void keyboard_set_jiggler(bool enabled);
bool keyboard_jiggler_enabled(void);
unsigned keyboard_mouse_pairs(void);
// Dev console only: queue one right/left pair using the real mouse worker.
bool keyboard_test_jiggle(void);
