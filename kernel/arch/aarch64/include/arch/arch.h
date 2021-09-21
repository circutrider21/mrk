#ifndef ARCH_H
#define ARCH_H

#include <stdint.h>

#define ARM64_WRITE_REG(reg, val) asm volatile ("msr " #reg ", %0;" :: "r" (val) : "memory");
#define ARM64_READ_REG(reg)  ({                            \
    uint64_t val = 0;                                               \
    asm volatile ("mrs %0, " #reg ";" :: "r" (val) : "memory"); \
    val;                                                        \
})

void arch_init_early();

void arch_init();

#endif // ARCH_H

