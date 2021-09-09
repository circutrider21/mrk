#ifndef ARCH_H
#define ARCH_H

#include <stdint.h>
#include <vm/virtual.h>

// Virtual Memory Constants -----------------------------------------
#define VM_PAGE_PRESENT  1
#define VM_PAGE_NOEXEC   (1ull << 54) | (1ull << 53)
#define VM_PAGE_READONLY (1 << 7)

typedef enum {
    CACHE_NOCACHE,
    CACHE_MMIO,
    CACHE_WRITE_COMBINE,
    CACHE_STANDARD
} cache_type;

void arch_invl_addr(const void* addr, uint32_t spid);
void arch_invl_spid(uint32_t spid);
void arch_map_4k(vm_aspace_t *spc, uintptr_t phys, uintptr_t virt, uint64_t flags, cache_type ca);

void arch_init_eardly();

#endif // ARCH_H

