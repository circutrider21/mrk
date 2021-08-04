#include <cstddef>
#include <cstdint>

#include <internal/lock.h>
#include <arch/arch.h>
#include <mrk/log.h>

// ring buffer for kernel log
static uint8_t log_buff[KLOG_BUFF_LEN];
static uint16_t log_start = 0;
static uint16_t log_end = 0;

// lock to prevent concurrent modification
static mutex log_mutex;

static void putch(uint8_t i)
{
    log_buff[log_end++] = i;
    if (log_end == log_start)
        log_start++;

    // Log to architecture specific debug port
    arch::log_char(i);
}

static void putsn(const char* s, uint64_t len)
{
    for (uint64_t i = 0; i < len; i++)
        putch(s[i]);
}

static void puts(const char* s)
{
    for (uint64_t i = 0; s[i] != '\0'; i++)
        putch(s[i]);
}

// print number as 64-bit hex
static void puthex(uint64_t n)
{
    puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uint64_t digit = (n >> i) & 0xF;
        putch((digit <= 9) ? (digit + '0') : (digit - 10 + 'A'));
    }
}

// print number
static void putint(int n)
{
    if (n == 0)
        putch('0');

    if (n < 0) {
        putch('-');
        n = -n;
    }
    size_t div = 1, temp = n;
    while (temp > 0) {
        temp /= 10;
        div *= 10;
    }
    while (div >= 10) {
        uint8_t digit = ((n % div) - (n % (div / 10))) / (div / 10);
        div /= 10;
        putch(digit + '0');
    }
}

static void vprintf(const char* s, va_list args)
{
    for (size_t i = 0; s[i] != '\0'; i++) {
        switch (s[i]) {
        case '%': {
            switch (s[i + 1]) {
            case '%':
                putch('%');
                break;

            case 'd':
                putint(va_arg(args, int));
                break;

            case 'x':
                puthex(va_arg(args, uint64_t));
                break;

            case 's':
                puts(va_arg(args, const char*));
                break;

            case 'b':
                puts(va_arg(args, int) ? "true" : "false");
                break;
            }
            i++;
        } break;

        default:
            putch(s[i]);
        }
    }
}

void log(char* fmt, ...)
{
    va_list v;
    va_start(v, fmt);

    // lock_retainer rk{log_mutex};
    vprintf(fmt, v);

    va_end(v);
}

void _todo(char* file, int line, char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    lock_retainer rk{log_mutex};

    // Print todo header
    puts("TODO(");
    puts(file);
    puts(":");
    putint(line);
    puts("): ");

    vprintf(fmt, va);

    va_end(va);
}

void flush_buffer()
{
    arch::log_str(log_buff, log_end);
}
