#include <internal/builtin.h>
#include <mrk/lock.h>
#include <mrk/log.h>

void mutex::lock()
{
    while (!__sync_bool_compare_and_swap(&status, false, true))
        ;
}

void mutex::unlock()
{
    __sync_bool_compare_and_swap(&status, true, false);
}

bool mutex::locked()
{
    return __sync_bool_compare_and_swap(&status, true, true);
}

// Various things required by the compiler

uintptr_t __stack_chk_guard = 0x941e9fac24fcb746;

__attribute__((noreturn)) extern "C" void __stack_chk_fail(void)
{
    PANIC("Stack smashing detected");
}

extern "C" void memset(uint64_t b, int c, int len)
{
    unsigned char* p = (unsigned char*)b;
    while (len > 0) {
        *p = c;
        p++;
        len--;
    }
    return;
}

extern "C" void memcpy(uint64_t dest, uint64_t src, size_t len)
{
    asm("rep movsd"
        :
        : "S"((uint64_t)src), "D"((uint64_t)dest), "c"(len)
        : "memory");
}
