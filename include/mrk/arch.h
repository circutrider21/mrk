#pragma once

#include <mrk/acpi.h>

#define CPUID_PCID (1 << 17)
#define CPUID_SMEP (1 << 7)
#define CPUID_SMAP (1 << 20)
#define CPUID_NX (1 << 20)
#define CPUID_PKU (1 << 3)
#define CPUID_PKS (1 << 31)

namespace arch {
void log_char(const uint8_t c);
void log_str(const uint8_t* str, uint64_t len);

bool cpuid(uint32_t leaf, uint32_t& eax, uint32_t& ebx, uint32_t& c, uint32_t& d);
bool cpuid(uint32_t leaf, uint32_t subleaf, uint32_t& eax, uint32_t& ebx, uint32_t& c, uint32_t& d);
uint64_t rdtsc();
}

// PIT Code ===================================================================

#define PIT_FREQUENCY 11931820
#define PIT_USEC 1000
#define PIT_MODE_COMMAND 0x43
#define PIT_DATA_PORT0 0x40
#define PIT_CHANNEL1 (1 << 5)
#define PIT_LOWBYTE (1 << 4)
#define PIT_SQUARE_WAVE 6

namespace arch {
void init_pit();
void sleep(uint64_t ms);
}

// Compiler Insintrics ========================================================

static inline uint8_t __inb(uint16_t port)
{
    uint8_t data;
    asm volatile("inb %1, %0"
                 : "=a"(data)
                 : "d"(port));
    return data;
}

static inline void __outb(uint16_t port, uint8_t data)
{
    asm volatile("outb %0, %1"
                 :
                 : "a"(data), "d"(port));
}

static inline uint16_t __inw(uint16_t port)
{
    uint16_t data;
    asm volatile("in %1, %0"
                 : "=a"(data)
                 : "d"(port));
    return data;
}

static inline void __outw(uint16_t port, uint16_t data)
{
    asm volatile("out %0, %1"
                 :
                 : "a"(data), "d"(port));
}

static inline uint32_t __ind(uint16_t port)
{
    uint32_t data;
    asm volatile("in %1, %0"
                 : "=a"(data)
                 : "d"(port));
    return data;
}

static inline void __outd(uint16_t port, uint32_t data)
{
    asm volatile("out %0, %1"
                 :
                 : "a"(data), "d"(port));
}

