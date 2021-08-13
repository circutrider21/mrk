#include <arch/arch.h>
#include <cstddef>
#include <mrk/log.h>

extern void (*term_write)(const char*, size_t length);

namespace arch {
void log_char(const uint8_t c)
{
    asm volatile("outb %1, %0"
                 :
                 : "dN"(0xE9), "a"(c));
}

void log_str(const uint8_t* str, uint64_t len)
{
    term_write((const char*)str, len);
}

bool cpuid(uint32_t leaf, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx)
{
    uint32_t cpuid_max;
    asm volatile("cpuid"
                 : "=a"(cpuid_max)
                 : "a"(leaf & 0x80000000)
                 : "rbx", "rcx", "rdx");

    if (leaf > cpuid_max) {
        log("CPUID failled. leaf:%x\n", leaf);
        return false;
    }

    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(leaf));

    return true;
}

bool cpuid(uint32_t leaf, uint32_t subleaf, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx)
{
    uint32_t cpuid_max;
    asm volatile("cpuid"
                 : "=a"(cpuid_max)
                 : "a"(leaf & 0x80000000)
                 : "rbx", "rcx", "rdx");

    if (leaf > cpuid_max) {
        log("CPUID failled. leaf:%x subleaf:%x\n", leaf, subleaf);
        return false;
    }

    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(leaf), "c"(subleaf));

    return true;
}

uint64_t rdtsc()
{
    uint64_t tsc_low = 0;
    uint64_t tsc_high = 0;
    asm("rdtsc"
        : "=a"(tsc_low), "=d"(tsc_high));
    return (tsc_low | (tsc_high << 32));
}
}

