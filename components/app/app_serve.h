#pragma once
// app_serve — what the hello-world app does with data another device POSTs to the knob (private
// to the app component). It only counts the bytes; replace the sink in app_serve.c with your own
// (parse a command, keep a file in PSRAM, ...). Depends on net only. Same hand-over as the
// upload: the server task publishes through atomics, the LVGL task polls them (see app.c).
#include <stdbool.h>
#include <stdint.h>

#define APP_SERVE_MAX_BODY (256u * 1024u)   // larger bodies are refused with 413

// Start the HTTP server with this app's sink. False if it could not start.
bool app_serve_start(void);

// True exactly once per completely received body, with its size in `*bytes`.
bool app_serve_poll(uint32_t *bytes);
