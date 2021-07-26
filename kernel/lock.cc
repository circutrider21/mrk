#include <mrk/lock.h>
#include <mrk/log.h>
#include <internal/builtin.h>

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

uintptr_t __stack_chk_guard = 0x941e9fac24fcb746;
 
__attribute__((noreturn))
extern "C" void __stack_chk_fail(void)
{
	PANIC("Stack smashing detected");
}

