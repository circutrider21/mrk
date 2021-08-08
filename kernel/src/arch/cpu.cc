#include <arch/cpu.h>
#include <cstdint>
#include <mrk/log.h>

uint64_t arch::cpu::rdmsr(uint32_t msr)
{
    uint32_t msrlow;
    uint32_t msrhigh;

    asm volatile("mov %[msr], %%ecx;"
                 "rdmsr;"
                 "mov %%eax, %[msrlow];"
                 "mov %%edx, %[msrhigh];"
                 : [msrlow] "=g"(msrlow), [msrhigh] "=g"(msrhigh)
                 : [msr] "g"(msr)
                 : "eax", "ecx", "edx");

    uint64_t msrval = ((uint64_t)msrhigh << 32) | msrlow;
    return msrval;
}

void arch::cpu::wrmsr(uint32_t msr, uint64_t val)
{
    uint32_t msrlow = val & UINT32_MAX;
    uint32_t msrhigh = (val >> 32) & UINT32_MAX;

    asm volatile("mov %[msr], %%ecx;"
                 "mov %[msrlow], %%eax;"
                 "mov %[msrhigh], %%edx;"
                 "wrmsr;"
                 :
                 : [msr] "g"(msr), [msrlow] "g"(msrlow), [msrhigh] "g"(msrhigh)
                 : "eax", "ecx", "edx");
}

SYSCALL(noop)
{
    log("proc: NOOP Syscall Called!\n");
}

extern "C" void syscall_handler();

void arch::cpu::init()
{
    // Set up syscall
    arch::cpu::wrmsr(0xc0000081, 0x0013000800000000);
    arch::cpu::wrmsr(0xc0000082, (uint64_t)syscall_handler);
    arch::cpu::wrmsr(0xc0000084, (uint64_t) ~((uint32_t)0x002)); // Enable interrupts
}
