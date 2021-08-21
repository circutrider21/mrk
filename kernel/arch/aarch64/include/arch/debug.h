#ifndef ARCH_DEBUG_H
#define ARCH_DEBUG_H

#include <arch/intr.h>

// Writes one character to the architecture specific debug port
void debug_putc(char c);

// Prints a debug dump of the cpu context to the console
void debug_dump(struct arch_cpu_frame* ctx, int el);

#endif // ARCH_DEBUG_H

