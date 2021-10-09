#ifndef ARCH_VMM_H
#define ARCH_VMM_H

#include <stdint.h>

#define ENCODE_ASID(x) ((uint64_t)(x) << 48)

static inline
void __invasid(uint32_t asid) {
  asm volatile ("dsb ishst");
  asm volatile ("tlbi aside1is, %0" : : "r"(ENCODE_ASID(asid)));

  asm volatile ("dsb ish; isb");
}

#endif // ARCH_VMM_H
