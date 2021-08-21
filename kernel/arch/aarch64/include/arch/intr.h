#ifndef ARCH_INTR_H
#define ARCH_INTR_H

#include <stdint.h>

// The size of a saved interrupt
#define STACK_FRAME_SIZE 288

#define arch_enable_irq() ({            \
    asm volatile ("msr daifclr, #2");   \
})

#define arch_disable_irq() ({           \
    asm volatile ("msr	daifset, #2");  \
})

struct arch_cpu_frame {
    uint64_t x[31];
    uint64_t sp;
    uint64_t elr;
    uint64_t spsr;
    uint64_t esr;
    uint64_t far;
};

#endif // ARCH_INTR_H

