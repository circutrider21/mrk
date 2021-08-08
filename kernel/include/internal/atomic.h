#pragma once

#include <cstdint>

// Some atomic functions...

template <typename T>
static inline bool atomic_cswap(T var, T cond, T val)
{
    bool to_ret;
    T ifthis = cond;

    asm volatile(
        "lock cmpxchg %0, %3"
        : "+m"(var), "+a"(ifthis),
        "=@ccz"(to_ret)
        : "r"(val)
        : "memory");

    return to_ret;
}

template <typename T>
static inline bool atomic_read(T var)
{
    T to_ret = 0;

    asm volatile(
        "lock xadd %0, %1"
        : "+r"(to_ret), "+m"(var)
        :
        : "memory");

    return to_ret;
}

template <typename T>
static inline bool atomic_write(T var, T val)
{
    T to_ret = val;

    asm volatile(
        "lock xchg %1, %0"
        : "+r"(to_ret), "+m"(var)
        :
        : "memory");

    return to_ret;
}

template <typename T>
static inline bool atomic_inc(T var)
{
    bool to_ret;

    asm volatile(
        "lock incq %1"
        : "=@ccnz"(to_ret)
        : "m"(var)
        : "memory");

    return to_ret;
}

template <typename T>
static inline bool atomic_dec(T var)
{
    bool to_ret;

    asm volatile(
        "lock dec %1"
        : "=@ccnz"(to_ret)
        : "m"(var)
        : "memory");

    return to_ret;
}
