#ifndef VM_VIRTUAL_H
#define VM_VIRTUAL_H

#include <stdint.h>

#define VM_PERM_READ   (1 << 0)
#define VM_PERM_WRITE  (1 << 1)
#define VM_PERM_EXEC   (1 << 2)
#define VM_PERM_USER   (1 << 3)
#define VM_PERM_GLOBAL (1 << 4)
#define VM_MAP_2MB     (1 << 5)

typedef struct vm_aspace {
    uintptr_t kroot;
    uintptr_t root;
    uint32_t spid;
} vm_aspace_t;

typedef enum {
    CACHE_NOCACHE,
    CACHE_MMIO,
    CACHE_WRITE_COMBINE,
    CACHE_STANDARD
} cache_type;

// Arch-Specific Virtual Memory Constants -----------------------------------------

#ifdef __aarch64__
#define VM_PAGE_PRESENT  1
#define VM_PAGE_NOEXEC   (1ull << 54) | (1ull << 53)
#define VM_PAGE_READONLY (1 << 7)
#define VM_PAGE_USER     (1 << 6)
#endif

void arch_invl_addr(const void* addr, uint32_t spid);
void arch_invl_spid(uint32_t spid);
void arch_map_4k(vm_aspace_t *space, uintptr_t phys, uintptr_t virt, uint64_t flags, cache_type ctype);
uint64_t arch_translate_virt(vm_aspace_t* spc, uintptr_t virt);

void vm_virt_map(vm_aspace_t* space, uintptr_t virt, uintptr_t phys, int flags, cache_type ctype);
void vm_virt_unmap(vm_aspace_t* space, uintptr_t virt);
void vm_virt_modify(vm_aspace_t* space, uintptr_t virt, int flags, cache_type ctype);

#define vm_v2p(s, v) ((uintptr_t)arch_translate_virt(s, v))
void vm_virt_setup(vm_aspace_t* space);

extern vm_aspace_t my_space;

#endif // VM_VIRTUAL_H
