#pragma once

#include <cstdint>

#define IA32_APIC 0x1B
#define IA32_X2APIC_BASE 0x800
#define LAPIC_TIMER_IRQ 32
#define IA32_TSC_DEADLINE 0x6e0

namespace arch::apic {
constexpr uint64_t lapic_spurious = 0x0f0;
constexpr uint64_t lapic_eoi = 0x0b0;
constexpr uint64_t lapic_icr0 = 0x300;
constexpr uint64_t lapic_icr1 = 0x310;
constexpr uint64_t lapic_timer = 0x320;

class xapic {
    uint8_t lapic_id;
    uint64_t tsc_freq;
    bool x2apic;
    bool is_bsp;
    bool tsc_supported;

    uint64_t read(uint32_t reg);
    void write(uint32_t reg, uint64_t val);

public:
    xapic();
    void eoi() { this->write(lapic_eoi, 0); }
    uint8_t get_id() { return this->lapic_id; }
    void send_ipi(uint8_t id, uint8_t vec);
    void timer_oneshot(uint64_t ms);

    friend void callibrate_tsc(arch::apic::xapic* apc);
};

void init();
void eoi();

void ipi(uint8_t lapic_id, uint8_t vec);
void ipi(xapic* apc, uint8_t vec);
}
