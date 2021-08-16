#include <arch/debug.h>
#include <lib/stivale2.h>

struct stivale2_struct_tag_mmio32_uart* mmio_struct;

void debug_putc(char c) {
    uint32_t write_val = (uint32_t)c;
    *((uint32_t*)mmio_struct->addr) = write_val;
}

