#include <klib/builtin.h>
#include <arch/arch.h>
#include <arch/cpu.h>
#include <mrk/log.h>
#include <mrk/alloc.h>

extern "C" void asm_xsave(uint8_t* state);
extern "C" void asm_xrstor(uint8_t* state);
using xsave_handler = void (*)(uint8_t* state);
using xrstor_handler = void (*)(uint8_t* state);

static xrstor_handler xrstor;
static xsave_handler xsave;
static uint64_t save_size = 0;
static uint64_t save_align = 0;

namespace arch::simd {
void init()
{
    uint64_t cr = 0;
    readcr("cr0", &cr);
    cr &= ~(1ull << 2); // Disable FPU Emulation
    cr |= (1 << 5); // Enable Native Exceptions
    cr |= (1 << 1); // Monitor Coprocessor
    writecr("cr0", cr);

    cr = 0;
    readcr("cr4", &cr);
    cr |= (1 << 9); // Enable FXSR
    cr |= (1 << 10); // Unmasked FPU Exception
    writecr("cr4", cr);

    uint32_t a, b, c, d;
    arch::cpuid(1, a, b, c, d);

    if (c & CPUID_XSAVE) {
        uint32_t a2, b2, c2, d2;
        if (!arch::cpuid(0xD, 0, a2, b2, c2, d2)) {
            PANIC("x64/fpu: XSAVE exists but CPUID dosen't support leaf 0xD\n");
        }

        cr = 0;
        readcr("cr4", &cr);
        cr |= (1 << 18);
        writecr("cr4", cr);

        save_size = c2;
        save_align = 64;
        xsave = asm_xsave;
        xrstor = asm_xrstor;

        uint64_t xcr0 = 0;
        xcr0 |= (1 << 0); // x87
        xcr0 |= (1 << 1); // SSE 1/2

        if (c & (1 << 28)) {
            xcr0 |= (1 << 2); // AVX
        }

        if (arch::cpuid(7, 0, a2, b2, c2, d2)) {
            if (b2 & (1 << 16)) {
                xcr0 |= (1 << 5); // AVX512
                xcr0 |= (1 << 6); // ZMM0-15
                xcr0 |= (1 << 7); // ZMM16-32
            }
        }

        wrxcr(0, xcr0);

        log("x64/fpu: Using XSAVE for SIMD State Control (Size: %d)\n", save_size);
    } else if (d & CPUID_FXSAVE) {
        save_size = 512;
        save_align = 16;

        xsave = +[](uint8_t* state) { asm("fxsave %0"
                                          :
                                          : "m"(*state)); };
        xrstor = +[](uint8_t* state) { asm("fxrstor %0"
                                           :
                                           : "m"(*state)); };

        log("x64/fpu: Using FXSAVE for SIMD State Control (Size: 512)\n");
    } else {
        PANIC("x64/fpu: No SIMD Support!\n");
    }
}

state::state()
{
    data = (uint8_t*)mm::alloc(save_size);
}

state::~state()
{
    mm::free((void*)data);
}

void state::save()
{
    xsave(data);
}

void state::restore()
{
    xrstor(data);
}
}
