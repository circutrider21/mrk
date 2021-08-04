#pragma once

#include <arch/cpu.h>

#define cur_cpu arch::cpu::current()

namespace arch::cpu {
class cpu_info {
public:
    uint64_t kernel_stack;
    uint64_t user_stack;
    uint64_t errno;
    int cpu_number;

    uint64_t timeslice;
    arch::apic::xapic* apc;

    void store() { arch::cpu::wrmsr(0xC0000102, (uint64_t)this); }
};

inline cpu_info* current() { return (cpu_info*)arch::cpu::rdmsr(0xC0000102); }
}

namespace smp {
void init_others();
arch::cpu::cpu_info* get(int index);
int count();
}

