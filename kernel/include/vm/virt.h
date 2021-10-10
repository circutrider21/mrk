#ifndef VM_VIRT_H
#define VM_VIRT_H

#include <vm/vm.h>
#include <stdint.h>

typedef uint64_t pte_t;
typedef uint64_t pde_t;

#define VM_PROT_NONE   0
#define VM_PROT_READ   1
#define VM_PROT_WRITE  2
#define VM_PROT_EXEC   4
#define VM_PROT_MASK   (1 | 2 | 4)

// Cache modes
#define VM_CACHE_MASK    (11 << 24)
#define VM_NOCACHE       (1 << 24)
#define VM_WRITE_THROUGH (2 << 24)
#define VM_WRITE_BACK    (3 << 24)
#define VM_DEVICE        (4 << 24)

// Map sizes
#define VM_MAP_2M   (1 << 10)
#define VM_MAP_1G   (2 << 10)
#define VM_MAP_MASK (3 << 10)

#define HIGHER_HALF(va) ((uint64_t)(va) + VM_MEM_OFFSET)

typedef struct {
  uint16_t asid;
  uintptr_t root_addr;
} vm_aspace_t;

void vm_virt_map(vm_aspace_t* space, uintptr_t phys, uintptr_t virt, uint64_t flags);
void vm_virt_unmap(vm_aspace_t* space, uintptr_t virt);
uintptr_t virt2phys(vm_aspace_t* space, uintptr_t phys);

#endif // VM_VIRT_H
