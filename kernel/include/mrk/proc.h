#pragma once

#include <arch/cpu.h>
#include <arch/idt.h>
#include <internal/lock.h>
#include <klib/vector.h>
#include <mrk/vmm.h>

namespace proc {
enum class thread_state : uint8_t { READY,
    BLOCKED,
    DEAD };
class process;

class thread {
public:
    thread_state tstate;
    process* prc;
    spinlock l;
    arch::idt::cpu_ctx context;
    uint64_t gs_base;
    uint64_t fs_base;
    uintptr_t kernel_stack;
    arch::simd::state simd_state;
    uint64_t timeslice;
};

class process {
public:
    int pid;
    int ppid;
    klib::vector<thread*> threads;
    klib::vector<process*> children;
    mm::vmm::aspace* space;
};

void init();
process* create_process();
thread* create_kthread(uint64_t ip, uint64_t arg0, bool start);
}
