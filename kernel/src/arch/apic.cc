#include <arch/apic.h>
#include <arch/hpet.h>
#include <arch/arch.h>
#include <arch/cpu.h>
#include <arch/idt.h>
#include <arch/smp.h>
#include <klib/builtin.h>

#include <mrk/alloc.h>
#include <mrk/log.h>
#include <mrk/pmm.h>
#include <mrk/vmm.h>

using namespace mm::vmm;
using namespace arch::cpu;
using namespace arch::apic;

// Calibration helpers

static bool check_tsc()
{
    // For a CPU to be considered supported it must have the following
    //    - TSC Deadline mode
    //    - Invariant TSC

    uint32_t a, b, c, d;

    // Check for TSC deadline
    arch::cpuid(1, 0, a, b, c, d);
    if (!(c & (1 << 24))) {
        return false;
    }

    // Check for invariant TSC
    arch::cpuid(0x80000007, 0, a, b, c, d);
    if (!(d & (1 << 8))) {
        return false;
    }

    return true;
}

// The default handler, sends EOI and sets up the next interrupt
static void simple_handler(arch::idt::cpu_ctx* cx, void* usrptr)
{
    // log("x64/lapic: No timer handler registered!\n");
}

uint64_t xapic::read(uint32_t reg)
{
    if (x2apic)
        return rdmsr(IA32_X2APIC_BASE + (reg >> 4));

    return *((volatile uint32_t*)(((rdmsr(IA32_APIC) & 0xfffff000) + MEM_VIRT_OFFSET) + reg));
}

void xapic::write(uint32_t reg, uint64_t v)
{
    if (this->x2apic) {
        wrmsr(IA32_X2APIC_BASE + (reg >> 4), v);
    } else {
        *((volatile uint32_t*)(((rdmsr(IA32_APIC) & 0xfffff000) + MEM_VIRT_OFFSET) + reg)) = v;
    }
}

void xapic::send_ipi(uint8_t lapic_id, uint8_t vec)
{
    if (this->x2apic) {
        this->write(lapic_icr0, ((uint64_t)lapic_id << 32) | (1 << 14) | vec);
    } else {
        this->write(lapic_icr1, ((uint32_t)lapic_id) << 24);
        this->write(lapic_icr0, vec);
    }
}

void xapic::timer_oneshot(uint64_t ms)
{
    uint64_t ticks = ms * (this->tsc_freq / 1000000);

    // Use TSC Deadline mode if supported
    if (this->tsc_supported) {
        this->write(lapic_timer, (0b10 << 17) | LAPIC_TIMER_IRQ);

        uint64_t target = arch::rdtsc() + ticks;

        wrmsr(IA32_TSC_DEADLINE, target);
    } else {
        this->write(lapic_timer, LAPIC_TIMER_IRQ);
        this->write(lapic_timer + 0xC0, 0); // Set divisor
        this->write(lapic_timer + 0x60, (uint32_t)ticks); // Set deadline
    }
}

xapic::xapic()
{
    uint32_t a, b, c, d;
    arch::cpuid(1, a, b, c, d);

    this->x2apic = ((c & (1 << 21)) != 0);

    auto base = rdmsr(IA32_APIC);
    base |= (1 << 11); // Set Enable bit
    base |= (this->x2apic << 10); // Set x2APIC bit if supported
    wrmsr(IA32_APIC, base);

    if (!x2apic) {
        auto mmio_base = (rdmsr(IA32_APIC) & 0xfffff000);
        mm::vmm::kernel_space()->map(mmio_base, mmio_base + MEM_VIRT_OFFSET, PAGE_PRESENT | PAGE_WRITEABLE | PAGE_NX, cache_type::nocache);
    }

    // Set spurious intr handler and enable all interrupt classes
    this->write(lapic_spurious,
        this->read(lapic_spurious) | (1 << 8) | 0xFF);
    this->write(0x80, 0);

    this->is_bsp = base & (1 << 8);
    this->lapic_id = (this->read(0x20) >> 24);

    if (this->is_bsp) {
        log("x64/lapic: Enabled in %s mode.\n", this->x2apic ? "x2APIC" : "xAPIC");
    }

    // Now, init the timer
    if (check_tsc()) {
        this->tsc_supported = true;
        callibrate_tsc(this);
    } else {
        // TODO: Add support for non-tsc time sources (like the standard lapic timer)
        PANIC("TSC is not supported!\n");
    }

    // Setup IRQ handler for timer and start chain of interrupts
    if (this->is_bsp)
        arch::idt::register_handler({ .vector = LAPIC_TIMER_IRQ, .callback = simple_handler, .is_irq = true, .should_iret = true, .is_timer = true });
    this->timer_oneshot(cur_cpu->timeslice);
}

namespace arch::apic {
void callibrate_tsc(xapic* apc)
{
    // Personally, I have found that callirating the TSC
    // is best done in 2 attempts, but feel free to change.

    int cycles = 2;

    for (int i = 0; i < cycles; i++) {
        uint64_t tsc_start = arch::rdtsc();

        // Wait 1 millisecond
	arch::hpet::sleep(1000);

        uint64_t tsc_end = arch::rdtsc();
        uint64_t freq = (tsc_end - tsc_start) * 1000;

        apc->tsc_freq += freq;
    }

    // Average out all readings
    apc->tsc_freq /= cycles;
    if (apc->is_bsp)
        log("x64/lapic: TSC Frequency is locked at %x Hz.\n", apc->tsc_freq);
}

void init()
{
    cur_cpu->apc = new xapic();
}

void eoi()
{
    cur_cpu->apc->eoi();
}

void send_ipi(uint8_t lapic_id, uint8_t vec)
{
    cur_cpu->apc->send_ipi(lapic_id, vec);
}
}
