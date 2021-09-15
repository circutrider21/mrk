#ifndef ARCH_TIMER_H
#define ARCH_TIMER_H

#include <generic/acpi.h>

#define NS_PER_S 1000000000

typedef struct {
    acpi_sdt header;
    uint64_t counter_control_block;
    uint32_t reserved;

    uint32_t s_el1_irq;
    uint32_t s_el1_flags;
    uint32_t ns_el1_irq;
    uint32_t ns_el1_flags;
    uint32_t v_el1_irq;
    uint32_t v_el1_flags;

    uint32_t ns_el2_irq;
    uint32_t ns_el2_flags;
    uint32_t pf_entries;
    uint32_t pf_offset;
    
    uint32_t v_el2_irq;
    uint32_t v_el2_flags;
} acpi_gtdt;

void timer_init();

void arch_sleep(uint32_t ms);

#endif // ARCH_TIMER_H
