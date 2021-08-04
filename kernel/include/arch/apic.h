#pragma once

#include <cstdint>

#define LAPIC_REG_TIMER_SPURIOUS 0x0f0
#define LAPIC_REG_EOI 0x0b0
#define LAPIC_REG_ICR0 0x300
#define LAPIC_REG_ICR1 0x310
#define LAPIC_REG_LVT_TIMER 0x320
#define LAPIC_REG_TIMER_INITCNT 0x380
#define LAPIC_REG_TIMER_CURRCNT 0x390
#define LAPIC_REG_TIMER_DIV 0x3e0

#define IA32_X2APIC 0x800
#define LAPIC_APIC_ID 0x20
#define LAPIC_LVT_TIMER_MODE_PERIODIC 0x20000
#define LAPIC_REGISTER_LVT_INT_MASKED 0x10000

namespace arch {
void init_apic();
void eoi();
void send_ipi(uint8_t lapic_id, uint8_t vec);
uint16_t get_lapic_id();

void init_timer();
}
