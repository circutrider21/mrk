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

// ====================================
//               PIT Code
// ====================================

static uint32_t read_counter()
{
    __outb(PIT_MODE_COMMAND, 0);
    uint32_t counter = __inb(PIT_DATA_PORT0);
    counter |= __inb(PIT_DATA_PORT0) << 8;

    return counter;
}

namespace arch {
void init_pit()
{
    int freq_divisor = 1000;
    uint16_t divisor = PIT_FREQUENCY / freq_divisor;

    __outb(PIT_MODE_COMMAND, PIT_CHANNEL1 | PIT_LOWBYTE | PIT_SQUARE_WAVE);
    __outb(PIT_DATA_PORT0, divisor & 0xFF);
    __outb(PIT_DATA_PORT0, (divisor >> 8) & 0xFF);
}

void sleep(uint64_t ms)
{
    uint16_t wait_val = (ms);

    __outb(PIT_MODE_COMMAND, PIT_CHANNEL1 | PIT_LOWBYTE);
    __outb(PIT_DATA_PORT0, wait_val & 0xFF);
    __outb(PIT_DATA_PORT0, (wait_val >> 8) & 0xFF);

    while (!(read_counter() == 0))
        ;
}
}
