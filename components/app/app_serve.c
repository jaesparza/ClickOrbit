// app_serve.c — the receiving side of the app↔net seam. The sink runs on the HTTP server task,
// so it does no LVGL and no blocking work: it notes that a body is complete and returns.
#include "app_serve.h"
#include "net_serve.h"
#include <stdatomic.h>

static atomic_uint_fast32_t s_bytes;
static atomic_bool          s_fresh;

static void on_data(const uint8_t *data, size_t len, size_t offset, size_t total)
{
    (void)data;   // this app only counts; copy from `data` here if you need the content
    if (offset + len == total) {
        atomic_store(&s_bytes, (uint_fast32_t)total);
        atomic_store(&s_fresh, true);
    }
}

bool app_serve_start(void)
{
    return net_serve_start(on_data, APP_SERVE_MAX_BODY);
}

bool app_serve_poll(uint32_t *bytes)
{
    if (!atomic_exchange(&s_fresh, false)) return false;
    *bytes = (uint32_t)atomic_load(&s_bytes);
    return true;
}
