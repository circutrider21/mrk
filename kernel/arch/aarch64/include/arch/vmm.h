#ifndef ARCH_VMM_H
#define ARCH_VMM_H

#include <stdint.h>
#include <vm/virt.h>

#define ENCODE_ASID(x) ((uint64_t)(x) << 48)

static inline
void __invasid(uint16_t asid) {
  asm volatile ("dsb ishst");
  asm volatile ("tlbi aside1is, %0" : : "r"(ENCODE_ASID(asid)));

  asm volatile ("dsb ish; isb");
}

static inline
void __invpg(uintptr_t va, uint16_t asid) {
  asm volatile ("dsb ishst");

  /*
   * vae1is - Invalidate translation used at EL1 for the specified VA and
   * Address Space Identifier (ASID) and the current VMID, Inner Shareable.
   * vaae1is - Invalidate all translations used at EL1 for the specified
   * address and current VMID and for all ASID values, Inner Shareable.
   *
   * Based on arm documentation
   * [https://developer.arm.com/documentation/ddi0488/h/system-control/aarch64-register-summary/aarch64-tlb-maintenance-operations]
   * and pmap_invalidate_page from FreeBSD - /sys/arm64/arm64/pmap.c.
   */
  if (asid) {
    asm volatile ("tlbi vae1is, %0" : : "r"(ENCODE_ASID(asid) | (va >> 12)));
  } else {
    /*
     * We don't know why but NetBSD, FreeBSD and Linux use vaae1is for kernel.
     * Using vae1is for kernel still works for us, but we don't want to leave
     * potential bug in that part of code.
     */
    asm volatile ("tlbi vaae1is, %0" : : "r"(va >> 12));
  }

  asm volatile ("dsb ish; isb");
}

// MAIR_EL1 - Memory Attributes Indirecction Register
#define MAIR_ATTR(attr, idx) ((attr) << ((idx)*8))
#define MAIR_DEVICE_nGnRnE 0x00
#define MAIR_NORMAL_NC 0x44
#define MAIR_NORMAL_WT 0xbb
#define MAIR_NORMAL_WB 0xff
#define MAIR_IDX(x) ((x) << 2)

// PTE cache indexes
#define ATTR_DEVICE_MEM    0
#define ATTR_NORMAL_MEM_NC 1
#define ATTR_NORMAL_MEM_WB 2
#define ATTR_NORMAL_MEM_WT 3

// Block and Page attributes
#define ATTR_UXN (1UL << 54)
#define ATTR_PXN (1UL << 53)
#define ATTR_XN (ATTR_PXN | ATTR_UXN)

/* Bits 58:55 are reserved for software according to the spec
 * Mrk uses those bits to save mapping type (read,write,execute) */

#define ATTR_SW_SHIFT 55
#define ATTR_SW_READ (1UL << ATTR_SW_SHIFT)
#define ATTR_SW_WRITE (2UL << ATTR_SW_SHIFT)
#define ATTR_SW_NOEXEC (4UL << ATTR_SW_SHIFT)
#define ATTR_nG (1 << 11)
#define ATTR_AF (1 << 10) /* Access Flag = 0: MMU faults on any access */
#define ATTR_AP_MASK (3 << 6)
#define ATTR_AP_RW (0 << 6)
#define ATTR_AP_RO (2 << 6)
#define ATTR_AP_USER (1 << 6)

/* Level 0 table, 512GiB per entry */
#define L0_SHIFT 39
#define L0_SIZE (1ul << L0_SHIFT)
#define L0_INVAL 0x0 /* An invalid address */
                     /* 0x1 Level 0 doesn't support block translation */
                     /* 0x2 also marks an invalid address */
#define L0_TABLE 0x3 /* A next-level table */

/* Level 1 table, 1GiB per entry */
#define L1_SHIFT 30
#define L1_SIZE (1 << L1_SHIFT)
#define L1_BLOCK 0x1
#define L1_TABLE L0_TABLE

/* Level 2 table, 2MiB per entry */
#define L2_SHIFT 21
#define L2_BLOCK 0x1
#define L2_TABLE L1_TABLE

/* Level 3 table, 4KiB per entry */
#define L3_SHIFT 12
#define L3_SIZE (1 << L3_SHIFT)
#define L3_SHIFT 12
#define L3_PAGE 0x3

// Page indexing macros
#define L0_INDEX(va) (((va) >> L0_SHIFT) & ((1 << 9) - 1))
#define L1_INDEX(va) (((va) >> L1_SHIFT) & ((1 << 9) - 1))
#define L2_INDEX(va) (((va) >> L2_SHIFT) & ((1 << 9) - 1))
#define L3_INDEX(va) (((va) >> L3_SHIFT) & ((1 << 9) - 1))
#define PTE_FRAME_ADDR(pte) ((pte)&0xfffffffff000)

// The actual functions
void arch_map_4k(vm_aspace_t* space, uintptr_t virt, uintptr_t phys, uint64_t flags);
void arch_map_2m(vm_aspace_t* space, uintptr_t virt, uintptr_t phys, uint64_t flags);
void arch_map_1g(vm_aspace_t* space, uintptr_t virt, uintptr_t phys, uint64_t flags);

#endif // ARCH_VMM_H
