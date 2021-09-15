#include <arch/timer.h>
#include <generic/log.h>
#include <stdbool.h>

static uint64_t freq;
static uint64_t freq_per_ms;

/*
 *  Usages of various timers...
 *
 *  Virtual - Used as a preemption timer
 *  Physical - Used as a wall clock for things like sleeping and callibration
*/

static uint64_t read_counter(bool virt) {
    uint64_t cval;

    if (virt) {
        asm volatile ("mrs %0, cntvct_el0" : "=r"(cval));
    } else {
        asm volatile ("mrs %0, cntpct_el0" : "=r"(cval));
    }

    return cval;
}

void timer_init() {
    acpi_gtdt* gtdt = (acpi_gtdt*)acpi_query("GTDT", 0);
    if (gtdt == NULL) {
        log("kernel/aarch64: GTDT not available, Extended setup will not occur.\n");
    } else {
        log("kernel/aarch64: Non-Secure EL1 Timer Interrrupt -> %d\n", gtdt->ns_el1_irq);
        log("kernel/aarch64: A total of %u additional timers (not used)\n", gtdt->pf_entries);
    }

    // Calculate Frequencies
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
	freq_per_ms = freq / 1000;

    // Unmask interrupts and enable counters
	asm volatile ("msr cntp_cval_el0, %0" :: "r"(0xFFFFFFFFFFFFFFFF));
	asm volatile ("msr cntp_ctl_el0, %0" :: "r"(1ull));
	asm volatile ("msr cntv_cval_el0, %0" :: "r"(0xFFFFFFFFFFFFFFFF));
	asm volatile ("msr cntv_ctl_el0, %0" :: "r"(1ull));
}

void arch_sleep(uint32_t ms) {
    uint64_t dest_ticks = ms * freq_per_ms;
    while (read_counter(false) != dest_ticks);
}
