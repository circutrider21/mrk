#pragma once

#include <arch/apic.h>
#include <arch/cpu.h>
#include <mrk/proc.h>

#define cur_cpu ((cpu_info*)arch::cpu::rdmsr(0xC0000102))

namespace arch::cpu {

class cpu_info {
public:
    uint64_t kernel_stack;
    uint64_t user_stack;
    uint64_t errno;
    int cpu_number;

    uint64_t timeslice;
    arch::apic::xapic* apc;
    proc::thread* current_thread;

    void store() { arch::cpu::wrmsr(0xC0000102, (uint64_t)this); }
};

} // namespace arch::cpu

namespace smp {

void init_others();
arch::cpu::cpu_info* get(int index);
int count();

} // namespace smp
