#include <arch/apic.h>
#include <arch/idt.h>
#include <arch/smp.h>
#include <mrk/log.h>

static arch::idt::IDT kidt;
static arch::idt::IDTR kidtr;

extern "C" void* __interrupt_vector[];
static uint16_t lastvector = 64;

arch::idt::callback handlers[256] = {};

extern "C" void __spurious();

static const char* __exmes[32] = {
    "Division by zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Detected overflow",
    "Out-of-bounds",
    "Invalid opcode",
    "No coprocessor",
    "Double fault",
    "Coprocessor segment overrun",
    "Bad TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Unknown interrupt",
    "Coprocessor fault",
    "Alignment check",
    "Machine check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
};

#define IDT_FLAGS_DEFAULT 0b1000111000000000

namespace arch::idt {
idt_entry::idt_entry(void* handler, uint16_t selector, bool present, uint8_t ist_num, uint8_t dpl)
{
    if (ist_num > IST_MAX)
        return;

    uint64_t ptr = (uint64_t)handler;

    this->low = (ptr & 0xFFFF);
    this->mid = ((ptr >> 16) & 0xFFFF);
    this->high = ((ptr >> 32) & 0xFFFFFFFF);
    this->reserved = 0;
    this->gdt_sel = selector;

    uint16_t flags = IDT_FLAGS_DEFAULT;

    if (dpl)
        flags |= ((dpl & 0x3) << 13);

    this->options = flags;
}

idt_entry::idt_entry(void* handler, uint16_t selector, bool present, uint8_t ist_num)
{
    if (ist_num > IST_MAX)
        return;

    uint64_t ptr = (uint64_t)handler;

    this->low = (ptr & 0xFFFF);
    this->mid = ((ptr >> 16) & 0xFFFF);
    this->high = ((ptr >> 32) & 0xFFFFFFFF);
    this->reserved = 0;
    this->gdt_sel = selector;

    uint16_t flags = IDT_FLAGS_DEFAULT;

    this->options = flags;
}

void init()
{
    for (int i = 0; i < 95; i++)
        kidt.set_entry(i, __interrupt_vector[i], 0x08, IST_NORMAL);

    // APIC Spurious Handler
    kidt.set_entry(255, (void*)__spurious, 0x08, IST_NORMAL);

    kidtr.base = (uint64_t)&kidt;
    kidtr.sz = (sizeof(idt_entry) * 256) - 1;

    kidtr.store();
}

void load()
{
    kidtr.store();
}

uint8_t free_vector()
{
    lastvector++;
    if (lastvector == 256)
        log("ERROR: Out of IRQ vectors\n");
    return 255;

    return lastvector;
}

void register_handler(callback h)
{
    handlers[h.vector] = h;
}

extern "C" uint64_t handle_isr(uint64_t sp)
{
    cpu_ctx* registers = (cpu_ctx*)sp;
    uint8_t n = registers->int_number & 0xFF;

    if (n < 32) {
        log("\nCPU: Exception #%d (%s)\n    Error Code: %x\n", n, __exmes[n], registers->error_code);
        log("    RIP: %x, RSP: %x\n", registers->rip, registers->rsp);
        log("    RAX: %x, RBX: %x, RCX: %x, RDX: %x\n", registers->rax, registers->rbx, registers->rcx, registers->rbx);
        log("    RSI: %x, RDI: %x, RSP: %x, RBP: %x\n", registers->rsi, registers->rdi, registers->rsp, registers->rbp);
        log("    R8: %x, R9: %x, R10: %x, R11: %x\n", registers->r8, registers->r9, registers->r10, registers->r11);
        log("    R12: %x, R12: %x, R13: %x, R14: %x\n", registers->r12, registers->r13, registers->r13, registers->r14);
        log("    R15: %x, CS: %x, SS: %x\n", registers->r15, registers->cs, registers->ss);
    }

    if (handlers[n].callback) {
        handlers[n].callback(registers, handlers[n].userptr);
    }

    if (handlers[n].is_irq)
        arch::apic::eoi();

    if (!handlers[n].should_iret && !handlers[n].is_irq)
        asm volatile("cli; hlt");

    if(handlers[n].is_timer)
	cur_cpu->apc->timer_oneshot(cur_cpu->timeslice);

    return sp;
}
}
