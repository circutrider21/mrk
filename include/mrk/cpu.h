#pragma once

#include <mrk/idt.h>

#define readcr(cr, n) asm volatile("mov %%" cr ", %%rax;" \
                                   "mov %%rax, %0 "       \
                                   : "=g"(*(n))           \
                                   :                      \
                                   : "rax");

#define writecr(cr, n) asm volatile("mov %0, %%rax;"   \
                                    "mov %%rax, %%" cr \
                                    :                  \
                                    : "g"(n)           \
                                    : "rax");

static inline void wrxcr(uint32_t reg, uint64_t value)
{
    uint32_t edx = value >> 32;
    uint32_t eax = (uint32_t)value;
    asm volatile("xsetbv"
                 :
                 : "a"(eax), "d"(edx), "c"(reg)
                 : "memory");
}

#define SYSCALL(name) extern "C" void __syscall_##name(arch::idt::cpu_ctx* context)

namespace arch::cpu {
uint64_t rdmsr(uint32_t msr);
void wrmsr(uint32_t msr, uint64_t val);
void init();
}

// GDT Decls and Code...

#define GDT_ENTRY_NULL 0x0000000000000000
#define GDT_ENTRY_KERNEL_CODE 0x00AF9A000000FFFF
#define GDT_ENTRY_KERNEL_DATA 0x008F92000000FFFF
#define GDT_ENTRY_USER_CODE 0x00AFFA000000FFFF
#define GDT_ENTRY_USER_DATA 0x008FF2000000FFFF

struct __attribute__((packed)) sys_seg_desc {
    uint16_t seg_limit_0_15;
    uint16_t base_addr_0_15;
    uint8_t base_addr_16_23;
    uint8_t flags_low;
    uint8_t flags_high;
    uint8_t base_addr_24_31;
    uint32_t base_addr_32_63;
    uint32_t reserved;
};

struct __attribute__((packed)) gdtr {
    uint16_t limit;
    uint64_t base;

    void load();
};

struct __attribute__((packed)) gdt {
    uint64_t entry_null;
    uint64_t entry_kcode;
    uint64_t entry_kdata;
    uint64_t entry_ucode;
    uint64_t entry_udata;
    sys_seg_desc tss;

    void init();
    // void load_tss(void* tss);

private:
    gdtr gt;
};

extern gdt kgdt;
