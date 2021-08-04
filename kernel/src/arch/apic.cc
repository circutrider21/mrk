#include <arch/apic.h>
#include <arch/arch.h>
#include <arch/cpu.h>
#include <arch/idt.h>
#include <mrk/log.h>
#include <mrk/pmm.h>
#include <mrk/vmm.h>

using namespace mm::vmm;
using namespace arch::cpu;

static bool x2apic = false;

static uint64_t lapic_read(uint32_t reg)
{
    if (x2apic)
        return rdmsr(IA32_X2APIC + (reg >> 4));
    else
        return *((volatile uint32_t*)(((rdmsr(0x1b) & 0xfffff000) + MEM_VIRT_OFFSET) + reg));
}

void lapic_write(uint32_t reg, uint64_t v)
{
    if (x2apic)
        return wrmsr(IA32_X2APIC + (reg >> 4), v);
    else
        *((volatile uint32_t*)(((rdmsr(0x1b) & 0xfffff000) + MEM_VIRT_OFFSET) + reg)) = v;
}

namespace arch {
void init_apic()
{
    uint32_t a, b, c, d;
    arch::cpuid(1, a, b, c, d);

    x2apic = ((c & (1 << 21)) != 0);

    auto base = rdmsr(0x1b);
    base |= (1 << 11); // Set Enable bit
    base |= (x2apic << 10); // Set x2APIC bit if supported
    wrmsr(0x1b, base);

    if (!x2apic) {
        auto mmio_base = (rdmsr(0x1b) & 0xfffff000);
        mm::vmm::kernel_space()->map(mmio_base, mmio_base + MEM_VIRT_OFFSET, PAGE_PRESENT | PAGE_WRITEABLE | PAGE_NX, cache_type::nocache);
    }

    lapic_write(LAPIC_REG_TIMER_SPURIOUS,
        lapic_read(LAPIC_REG_TIMER_SPURIOUS) | (1 << 8) | 0xFF);
    lapic_write(0x80, 0);

    if (base & (1 << 8))
        log("x64/lapic: Enabled in %s mode\n", x2apic ? "x2APIC" : "xAPIC"); // Print only if BSP

    arch::init_timer();
}

void eoi()
{
    lapic_write(LAPIC_REG_EOI, 0);
}

void send_ipi(uint8_t lapic_id, uint8_t vec)
{
    if (x2apic)
        lapic_write(LAPIC_REG_ICR0, ((uint64_t)lapic_id << 32) | (1 << 14) | vec);
    else
        lapic_write(LAPIC_REG_ICR1, ((uint32_t)lapic_id) << 24);
    lapic_write(LAPIC_REG_ICR0, vec);
}

uint16_t get_lapic_id()
{
    return (uint16_t)(lapic_read(LAPIC_APIC_ID) >> 24);
}
}

// a dummy handler, just sends eoi
static void dhandler(arch::idt::cpu_ctx* cx, void* usrptr)
{
    // log("x64/lapic: No timer handler registered\n");
}

void arch::init_timer()
{
    // The APIC Timer is IRQ 32
    arch::idt::register_handler({ .vector = 32, .callback = dhandler, .is_irq = true, .should_iret = true });

    lapic_write(LAPIC_REG_TIMER_DIV, 3); // Divide by 16
    lapic_write(LAPIC_REG_TIMER_INITCNT, 0xFFFFFFFF); /* Set the value to -1 */

    arch::sleep(10);

    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_REGISTER_LVT_INT_MASKED);

    uint32_t tick_in_10ms = 0xFFFFFFFF - lapic_read(LAPIC_REG_TIMER_CURRCNT);

    lapic_write(LAPIC_REG_LVT_TIMER, 32 | LAPIC_LVT_TIMER_MODE_PERIODIC);
    lapic_write(LAPIC_REG_TIMER_DIV, 3);
    lapic_write(LAPIC_REG_TIMER_INITCNT, tick_in_10ms);

    if (rdmsr(0x1b) & (1 << 8))
        log("x64/lapic: Timer frequency: %d Hz. Divisor: 16.\n", tick_in_10ms);
}
