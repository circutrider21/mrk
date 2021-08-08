#include <arch/smp.h>
#include <klib/queue.h>
#include <mrk/pmm.h>
#include <mrk/proc.h>
#include <mrk/vmm.h>

using namespace arch::idt;
using namespace arch::cpu;

static klib::queue<proc::thread*>* run_queue;
proc::process* kernel_process;
static spinlock proc_lock;

static proc::thread* get_next_thread()
{
    lock_retainer guard { proc_lock };
    if (run_queue->is_empty())
        return nullptr;

restart:
    proc::thread* to_return = run_queue->pop();

    switch (to_return->tstate) {
    case proc::thread_state::BLOCKED:
        // TODO: Check if thread can be unblocked
        run_queue->push(to_return);
        if (run_queue->is_empty())
            return nullptr;
        else
            goto restart;
        break;
    case proc::thread_state::DEAD:
        mm::free(to_return); // Delete the dead thread
        if (run_queue->is_empty())
            return nullptr;
        else
            goto restart;
        break;
    case proc::thread_state::READY:
        return to_return;
        break;
    }

    // Shouldn't reach here anyways
    return nullptr;
}

// Cpus go here to relax :-)
static void __idle()
{
    asm volatile("sti");
    for (;;) {
        asm volatile("hlt");
    }
}

void reschedule(cpu_ctx* ctx, void* usrptr)
{
    (void)usrptr;

    proc::thread* old_thrd = cur_cpu->current_thread;

    if (old_thrd != nullptr) {
        // Save old context
        // old_thrd->simd_state.save();
        old_thrd->context = *ctx;

        old_thrd->gs_base = rdmsr(0xc0000101);
        old_thrd->fs_base = rdmsr(0xc0000100);

        proc_lock.lock();
        run_queue->push(old_thrd);
        proc_lock.unlock();
    }

    proc::thread* new_thrd = get_next_thread();

    // Idle for now...
    if (new_thrd == nullptr) {
        ctx->cs = 0x08;
        ctx->rip = (uint64_t)&__idle;

        cur_cpu->current_thread = nullptr;
        return;
    }

    // Swap contexts
    wrmsr(0xc0000101, new_thrd->gs_base);
    wrmsr(0xc0000100, new_thrd->fs_base);
    // new_thrd->simd_state.restore();
    *ctx = new_thrd->context;

    // Update cpu_info for the current cpu
    cur_cpu->current_thread = new_thrd;
    cur_cpu->timeslice = new_thrd->timeslice;
    cur_cpu->kernel_stack = new_thrd->kernel_stack;

    return;
}

void proc::init()
{
    // Create the queue
    run_queue = new klib::queue<proc::thread*>(512);

    // Craft the first kernel process by hand
    kernel_process = new proc::process();
    kernel_process->space = mm::vmm::kernel_space();

    // Register the interrupt handler for the APIC timer IRQ
    register_handler({
        .vector = 32,
        .callback = &reschedule,
        .is_irq = true,
        .should_iret = true,
        .is_timer = true,
    });
}

proc::thread* proc::create_kthread(uint64_t ip, uint64_t arg0, bool start)
{
    uint64_t stack_ptr = (uint64_t)mm::pmm::get(2);

    mm::vmm::kernel_space()->map(stack_ptr, stack_ptr + MEM_VIRT_OFFSET, PAGE_PRESENT | PAGE_WRITEABLE | PAGE_NX);
    mm::vmm::kernel_space()->map(stack_ptr + 0x1000, (stack_ptr + 0x1000) + MEM_VIRT_OFFSET, PAGE_PRESENT | PAGE_WRITEABLE | PAGE_NX);

    uint64_t stack = stack_ptr + 8192 + MEM_VIRT_OFFSET;
    _memset(stack_ptr + MEM_VIRT_OFFSET, 0, 8192);

    proc::thread* thrd = new proc::thread();
    *thrd = (proc::thread) {
        .tstate = proc::thread_state::READY,
        .prc = kernel_process,
        .kernel_stack = stack_ptr + MEM_VIRT_OFFSET,
        .timeslice = 5000,
    };

    thrd->context.cs = 0x08;
    thrd->context.ss = 0x10;
    thrd->context.rflags = 0x202;
    thrd->context.rip = ip;
    thrd->context.rdi = arg0;
    thrd->context.rbp = 0; // Zero out so stack traces are accurate
    thrd->context.rsp = stack;

    if (start)
        run_queue->push(thrd);

    return thrd;
}
