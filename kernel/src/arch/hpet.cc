#include <arch/hpet.h>
#include <mrk/vmm.h>
#include <mrk/pmm.h>
#include <klib/builtin.h>
#include <mrk/log.h>

static void* hpet_regs;
static uint64_t hpet_period;

static uint64_t read_hpet(uint16_t offset)
{
    return *((volatile uint64_t*)((uint64_t)hpet_regs + offset));
}

static void write_hpet(uint16_t offset, uint64_t val)
{
    *((volatile uint64_t*)((uint64_t)hpet_regs + offset)) = val;
}

namespace arch::hpet {
    
    uint64_t get_nanos()
    {
        return read_hpet(main_counter_data) * hpet_period;
    }

    void sleep(uint64_t nanos)
    {
        uint64_t tgt = get_nanos() + nanos;
        while (get_nanos() < tgt)
            ;
    }

    void init()
    {
	arch::hpet::table* hpet_table = (arch::hpet::table*)acpi::get_table("HPET", 0);
        if (!hpet_table)
            PANIC("HPET is needed as a kernel timesource\n");

        // map the hpet registers
        uint64_t hpet_phys = hpet_table->base_addr.address;
	mm::vmm::kernel_space()->map(hpet_phys, hpet_phys + MEM_VIRT_OFFSET, PAGE_PRESENT | PAGE_WRITEABLE | PAGE_NX);
        hpet_regs = (void*)(hpet_phys + MEM_VIRT_OFFSET);

        // Get the time period
        hpet_period = (read_hpet(cap_id) >> 32) / 1000000;

        // Enable the hpet
        write_hpet(gen_conf, read_hpet(gen_conf) | (1 << 0));
        log("hpet: Enabled! (Time Period -> %x)\n", hpet_period);
    }
}

