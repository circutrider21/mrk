#include <internal/lock.h>
#include <klib/builtin.h>
#include <mrk/log.h>

// Defined in arch/funcs.asm
extern "C" void __spinlock_lock(uint16_t* lk);
extern "C" void __spinlock_unlock(uint16_t* lk);
extern "C" bool __spinlock_try(uint16_t* lk);

void mutex::lock()
{
    while (!__sync_bool_compare_and_swap(&status, false, true))
        ;
}

void mutex::unlock()
{
    __sync_bool_compare_and_swap(&status, true, false);
}

bool mutex::try_lock()
{
    if (__sync_bool_compare_and_swap(&status, true, true)) {
        this->lock();
        return true;
    } else {
        return false;
    }
}

void spinlock::lock()
{
    __spinlock_lock(&_lock);
}

void spinlock::unlock()
{
    __spinlock_unlock(&_lock);
}

bool spinlock::try_lock()
{
    return __spinlock_try(&_lock);
}

// Various things required by the compiler
// I keep them here for now...

uintptr_t __stack_chk_guard = 0x941e9fac24fcb746;

extern "C" void __stack_chk_fail(void)
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
    _memcpy((void*)dest, (void*)src, len);
}
