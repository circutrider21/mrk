#include <vm/virt.h>
#include <arch/vmm.h>

void vm_virt_map(vm_aspace_t* space, uintptr_t phys, uintptr_t virt, uint64_t flags) {
  int paging_mode = flags & VM_MAP_MASK;

  if (paging_mode == 1) // 2MB Mapping
    arch_map_2m(space, phys, virt, flags);
  else if (paging_mode == 2) // 1GB Mapping
    arch_map_1g(space, phys, virt, flags);
  else { // 4KB Mapping
    log("p2: 0x%p -> v2: 0x%p\n", phys, virt);
    arch_map_4k(space, virt, phys, flags);
  }
}
