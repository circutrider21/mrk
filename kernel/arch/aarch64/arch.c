#include <arch/arch.h>
#include <arch/dtb.h>

extern void __load_vectors();
void arch_init_early() {
    __load_vectors();
    init_dtb();
}

