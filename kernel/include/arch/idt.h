#pragma once

#include <cstdint>

#define IST_NORMAL 0
#define IST_DOUBLE_FAULT 1
#define IST_PAGE_FAULT 2
#define IST_NMI 3

#define IST_MAX 7

namespace arch::idt {
constexpr uint8_t idt_flag_present = 15;

void init();
void load();

uint8_t free_vector();

struct __attribute__((packed)) idt_entry {
    idt_entry() = default;
    idt_entry(void* handler, uint16_t selector, bool present, uint8_t ist_num, uint8_t dpl);
    idt_entry(void* handler, uint16_t, bool present, uint8_t ist_num);

    uint16_t low;
    uint16_t gdt_sel;
    uint16_t options;
    uint16_t mid;
    uint32_t high;
    uint32_t reserved;
};

struct __attribute__((packed)) IDT {
    IDT()
    {
        for (auto& g : entries) {
            g = idt_entry(nullptr, 0x08, false, 0);
        }
    }
    void set_entry(uint16_t n, void* func, uint16_t sel, uint8_t ist_index)
    {
        this->entries[n] = idt_entry(func, sel, true, ist_index);
    }

    void set_entry(uint16_t n, void* func, uint16_t sel, uint8_t ist_index, uint8_t dpl)
    {
        this->entries[n] = idt_entry(func, sel, true, ist_index, dpl);
    }

private:
    idt_entry entries[256];
};

struct __attribute__((packed)) IDTR {
    void store() { asm("lidt %0"
                       :
                       : "m"(*this)
                       : "memory"); }

    volatile uint16_t sz;
    volatile uint64_t base;
};

struct __attribute__((packed)) ctrsx {
    uint64_t ds;
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rdi, rsi, rbp, useless, rbx, rdx, rcx, rax;
    uint64_t int_number, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

struct __attribute__((packed)) cpu_ctx {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_number, error_code; // Pushed by handler
    uint64_t rip, cs, rflags, rsp, ss; // Pushed by cpu
};

class callback {
public:
    using idt_func = void (*)(cpu_ctx*, void*);
    uint16_t vector;
    idt_func callback;
    void* userptr = nullptr;
    bool is_irq = false, should_iret = false, is_timer = false;
};

void register_handler(callback h);
}
