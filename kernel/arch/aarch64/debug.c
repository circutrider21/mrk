#include <arch/debug.h>
#include <lib/stivale2.h>
#include <generic/log.h>

struct stivale2_struct_tag_mmio32_uart* mmio_struct;

void debug_putc(char c) {
    uint32_t write_val = (uint32_t)c;
    uint32_t *mmio = (uint32_t *)mmio_struct->addr;
    *((uint32_t*)mmio) = write_val;
}

void debug_dump(struct arch_cpu_frame* ctx, int el) {
    for(int i = 0; i < 31; i += 4) {
        log("  X%d: %p, X%d: %p, X%d: %p, X%d: %p\n",
		i, ctx->x[i], i+1, ctx->x[i+1], i+2, ctx->x[i+2], i+3, ctx->x[i+3]);
    }

    log("  SP: %p, ELR: %p, SPSR: %p\n", ctx->sp, ctx->elr, ctx->spsr);
    log("  ESR: %p, FAR: %p, ", ctx->esr, ctx->far);

    if(el == -1) {
        log("EL: (unknown)\n");
    } else {
        log("EL: (EL%d)\n", el);
    }
}

