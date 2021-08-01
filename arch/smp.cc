#include <internal/stivale2.h>
#include <mrk/apic.h>
#include <mrk/arch.h>
#include <mrk/idt.h>
#include <mrk/pmm.h>
#include <mrk/smp.h>
#include <mrk/vmm.h>

#include <cstddef>
#include <mrk/alloc.h>
#include <internal/lock.h>
#include <internal/atomic.h>
#include <mrk/log.h>

using namespace arch::cpu;
using namespace arch::idt;

static struct stivale2_struct_tag_smp* tag_smp;
static cpu_info* cpu_locals;
static spinlock smp_lock;
static uint64_t active_cpus = 1; // The BSP is already active

static void init_ap(struct stivale2_smp_info* blog)
{
    cpu_info* my_log = (cpu_info*)blog->extra_argument;
    my_log->store();

    smp_lock.lock();

    mm::vmm::check_and_init(false);
    mm::vmm::kernel_space()->load();
    kgdt.init();
    arch::idt::init();

    arch::init_apic();

    smp_lock.unlock();

    // log("smp: CPU #%d online!\n", arch::cpu::current()->cpu_number);
    atomic_inc<uint64_t>(active_cpus);

    asm volatile("sti");
    for (;;) {
        asm volatile("sti");
    }
}

static void init_local(int cpu_num)
{
    cpu_info* cf = &cpu_locals[cpu_num];
    cf->stac_supported = 0;
    cf->current_thread = nullptr;
    cf->queue_index = -1;
    cf->errno = 1; // Start with no errors

    cf->cpu_number = cpu_num;
}

namespace smp {
void init_others()
{
    tag_smp = (stivale2_struct_tag_smp*)arch::stivale2_get_tag(STIVALE2_STRUCT_TAG_SMP_ID);

    log("smp: BSP #%d | Total CPUs %d\n", tag_smp->bsp_lapic_id, tag_smp->cpu_count);

    cpu_locals = (cpu_info*)mm::alloc(sizeof(cpu_info) * tag_smp->cpu_count);

    init_local(0);
    cpu_locals[0].store();

    for (size_t i = 0; i < tag_smp->cpu_count; i++) {
        if (i == tag_smp->bsp_lapic_id)
            continue;
        tag_smp->smp_info[i].extra_argument = (uint64_t)&cpu_locals[i];
        uint64_t stack = (uintptr_t)mm::pmm::get(1) + MEM_VIRT_OFFSET;
        init_local(i);
        tag_smp->smp_info[i].target_stack = stack;
        tag_smp->smp_info[i].goto_address = (uint64_t)init_ap;
    }

    while (atomic_read<uint64_t>(active_cpus) != tag_smp->cpu_count)
        ;
}

cpu_info* get(int index) { return &cpu_locals[index]; }
int count() { return tag_smp->cpu_count; }
}
