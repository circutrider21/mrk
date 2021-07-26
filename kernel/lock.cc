#include <mrk/lock.h>

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
