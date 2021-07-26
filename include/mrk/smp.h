#pragma once

#include <mrk/cpu.h>
#include <mrk/proc.h>

namespace arch::cpu {
class cpu_info {
public:
    uint64_t kernel_stack;
    uint64_t user_stack;
    uint64_t errno;
    uint64_t stac_supported;
    int cpu_number;
    proc::thread* current_thread;
    int64_t queue_index;

    void store() { arch::cpu::wrmsr(0xC0000102, (uint64_t)this); }
};

inline cpu_info* current() { return (cpu_info*)arch::cpu::rdmsr(0xC0000102); }
}

namespace smp {
void init_others();
arch::cpu::cpu_info* get(int index);
int count();
}
