#pragma once

#include <cstdint>
#include <klib/vector.h>
#include <mrk/alloc.h>
#include <mrk/idt.h>
#include <mrk/lock.h>
#include <mrk/vmm.h>

// The max number of running threads, change with caution
#define MAX_THREADS 4096

// Default thread stack size (has to be a multiple of 4096)
#define THREAD_STACK_SIZE 8192

using pid_t = uint16_t;
using tid_t = uint16_t;

namespace proc {
class process;

class thread {
public:
    tid_t tid;
    mutex lock;

    bool is_queued;
    bool running;
    size_t timeslice;
    arch::idt::cpu_ctx ctx;
    process* pr;

    uintptr_t user_gs;
    uintptr_t user_fs;
    uintptr_t user_stack;
    uintptr_t kernel_stack;
};

class process {
public:
    pid_t id;
    pid_t pid;

    mm::vmm::aspace* space;
    klib::vector<thread*> threads;
    klib::vector<process*> children;
    int status;

    uintptr_t thread_stack_top;
};

void init();
thread* create_thread(thread* new_thread, process* proc, void* addr, void* arg, bool start, mm::vmm::aspace* new_space);
}

extern proc::process* kproc;

// ====================================
//        SIMD Support and Decls
// ====================================

#define CPUID_XSAVE 0x04000000
#define CPUID_FXSAVE 0x01000000

namespace arch::simd {
void init();

class state {
public:
    state();
    ~state();
    void save();
    void restore();

private:
    uint8_t* data;
};
}
