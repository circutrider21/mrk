#pragma once

#include <cstdint>

#define LOCKED_READ(VAR) ({    \
    __typeof__(VAR) ret = 0;   \
    asm volatile(              \
        "lock xadd %0, %1"     \
        : "+r"(ret), "+m"(VAR) \
        :                      \
        : "memory");           \
    ret;                       \
})

#define LOCKED_WRITE(VAR, VAL) ({ \
    __typeof__(VAR) ret = VAL;    \
    asm volatile(                 \
        "lock xchg %0, %1"        \
        : "+r"(ret), "+m"(VAR)    \
        :                         \
        : "memory");              \
    ret;                          \
})

#define LOCKED_INC(VAR) ({ \
    bool ret;              \
    asm volatile(          \
        "lock incq %1"     \
        : "=@ccnz"(ret)    \
        : "m"(VAR)         \
        : "memory");       \
    ret;                   \
})

#define LOCKED_DEC(VAR) ({ \
    bool ret;              \
    asm volatile(          \
        "lock decq %1"     \
        : "=@ccnz"(ret)    \
        : "m"(VAR)         \
        : "memory");       \
    ret;                   \
})

#define COMPARE_SWAP(HERE, IFTHIS, WRITETHIS) ({ \
    bool ret;                                    \
    __typeof__(IFTHIS) ifthis = IFTHIS;          \
    asm volatile(                                \
        "lock cmpxchgq %3, %0"                   \
        : "+m"(HERE), "+a"(ifthis),              \
        "=@ccz"(ret)                             \
        : "r"(WRITETHIS)                         \
        : "memory");                             \
    ret;                                         \
})

class mutex {
public:
    void lock();
    void unlock();
    bool locked();

private:
    int status;
};
