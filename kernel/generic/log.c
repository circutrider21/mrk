#include <generic/log.h>
#include <generic/fbcon.h>
#include <arch/debug.h>

// Temporary buffer for vsnprintf
static char buf[512];

static void log_puts(char* str, size_t bytes) {
    // TODO: Implement fbcon
    fbcon_puts(str);
    for(size_t cnt = 0; cnt < bytes; cnt++) {
        debug_putc(str[cnt]);
    }
}

void log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int bytes_used = vsnprintf(buf, 512, fmt, args);

    log_puts(buf, bytes_used);
    va_end(args);
}

