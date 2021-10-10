#include <arch/vmm.h>
#include <arch/arch.h>
#include <vm/phys.h>
#include <stddef.h>

#define PAGE_OFFSET(x) ((x) & (4096 - 1))

static const pte_t pte_common = L3_PAGE | (3 << 8);
static const pte_t pte_noexec = ATTR_XN | ATTR_SW_NOEXEC;
static const pte_t vm_prot_map[] = {
    [VM_PROT_NONE] = pte_noexec | pte_common,
    [VM_PROT_READ] =
        ATTR_AP_RO | ATTR_SW_READ | ATTR_AF | pte_noexec | pte_common,
    [VM_PROT_WRITE] =
        ATTR_AP_RW | ATTR_SW_WRITE | ATTR_AF | pte_noexec | pte_common,
    [VM_PROT_READ | VM_PROT_WRITE] = ATTR_AP_RW | ATTR_SW_READ | ATTR_SW_WRITE |
                         ATTR_AF | pte_noexec | pte_common,
    [VM_PROT_EXEC] = ATTR_AF | pte_common,
    [VM_PROT_READ | VM_PROT_EXEC] =
        ATTR_AP_RO | ATTR_SW_READ | ATTR_AF | pte_common,
    [VM_PROT_WRITE | VM_PROT_EXEC] =
        ATTR_AP_RW | ATTR_SW_WRITE | ATTR_AF | pte_common,
    [VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC] =
        ATTR_AP_RW | ATTR_SW_READ | ATTR_SW_WRITE | ATTR_AF | pte_common,
};
uint64_t kernel_root;

// Some helper functions...
static inline
uint64_t get_space(vm_aspace_t* space, uintptr_t va) {
  if ((va >> 63) & 1) {
    return kernel_root;
  } else {
    return space->root_addr;
  }
}

static pte_t make_pte(uintptr_t pa, pte_t prot, unsigned flags) {
  pte_t pte = pa | prot;
  unsigned cacheflags = flags & VM_CACHE_MASK;
  if (cacheflags == VM_NOCACHE)
    return pte | MAIR_IDX(ATTR_NORMAL_MEM_NC);
  if (cacheflags == VM_WRITE_THROUGH)
    return pte | MAIR_IDX(ATTR_NORMAL_MEM_WT);
  if (cacheflags == VM_DEVICE)
    return pte | MAIR_IDX(ATTR_DEVICE_MEM);
  return pte | MAIR_IDX(ATTR_NORMAL_MEM_WB);
}

static pte_t *pmap_lookup_pte(vm_aspace_t *spc, uintptr_t va) {
  pde_t *pdep;
  uintptr_t pa = get_space(spc, va);

  /* Level 0 */
  pdep = (pde_t *)HIGHER_HALF(pa) + L0_INDEX(va);
  if (!(pa = PTE_FRAME_ADDR(*pdep)))
    return NULL;

  /* Level 1 */
  pdep = (pde_t *)HIGHER_HALF(pa) + L1_INDEX(va);
  if (!(pa = PTE_FRAME_ADDR(*pdep)))
    return NULL;

  /* Level 2 */
  pdep = (pde_t *)HIGHER_HALF(pa) + L2_INDEX(va);
  if (!(pa = PTE_FRAME_ADDR(*pdep)))
    return NULL;

  /* Level 3 */
  return (pde_t *)HIGHER_HALF(pa) + L3_INDEX(va);
}

static pte_t *pmap_ensure_pte(vm_aspace_t *spc, uintptr_t va) {
  pde_t *pdep;
  uintptr_t pa = get_space(spc, va);
  
  /* Level 0 */
  pdep = (pde_t *)HIGHER_HALF(pa) + L0_INDEX(va);
  if (!(pa = PTE_FRAME_ADDR(*pdep))) {
    pa = (uintptr_t)vm_phys_alloc(1, VM_PAGE_ZERO);
    *pdep = pa | L0_TABLE;
  }

  /* Level 1 */
  pdep = (pde_t *)HIGHER_HALF(pa) + L1_INDEX(va);
  if (!(pa = PTE_FRAME_ADDR(*pdep))) {
    pa = (uintptr_t)vm_phys_alloc(1, VM_PAGE_ZERO);
    *pdep = pa | L1_TABLE;
  }

  /* Level 2 */
  pdep = (pde_t *)HIGHER_HALF(pa) + L2_INDEX(va);
  if (!(pa = PTE_FRAME_ADDR(*pdep))) {
    pa = (uintptr_t)vm_phys_alloc(1, VM_PAGE_ZERO);
    *pdep = pa | L2_TABLE;
  }

  /* Level 3 */
  return (pde_t *)HIGHER_HALF(pa) + L3_INDEX(va);
}

static void pmap_write_pte(vm_aspace_t *space, pte_t *ptep, pte_t pte, uintptr_t va) {
  if (!((va >> 63) & 1)) {
    pte |= ATTR_AP_USER | ATTR_PXN; // user mode memory can't be executed in supervisor mode
  }

  *ptep = pte;
  __invpg(va, (!((va >> 63) & 1)) ? space->asid : 0);
}

void arch_map_4k(vm_aspace_t* space, uintptr_t virt, uintptr_t phys, uint64_t flags) {
  log("p3: 0x%p -> v3: 0x%p\n", phys, virt);
  pte_t new_pte = make_pte(phys, vm_prot_map[flags & VM_PROT_MASK], flags & VM_CACHE_MASK);
  pte_t* slot = pmap_ensure_pte(space, virt);
  pmap_write_pte(space, slot, new_pte, virt);
}

void arch_map_2m(vm_aspace_t* space, uintptr_t virt, uintptr_t phys, uint64_t flags) {
  pde_t *pdep;
  uintptr_t pa = get_space(space, virt);
  pte_t new_pte = make_pte(phys, vm_prot_map[flags & VM_PROT_MASK], flags & VM_CACHE_MASK);

  // Remove L3_PAGE bit -> becomes L2_BLOCK
  new_pte &= ~(1 << 1);

  /* Level 0 */
  pdep = (pde_t *)HIGHER_HALF(pa) + L0_INDEX(virt);
  if (!(pa = PTE_FRAME_ADDR(*pdep))) {
    pa = (uintptr_t)vm_phys_alloc(1, VM_PAGE_ZERO);
    *pdep = pa | L0_TABLE;
  }

  /* Level 1 */
  pdep = (pde_t *)HIGHER_HALF(pa) + L1_INDEX(virt);
  if (!(pa = PTE_FRAME_ADDR(*pdep))) {
    pa = (uintptr_t)vm_phys_alloc(1, VM_PAGE_ZERO);
    *pdep = pa | L1_TABLE;
  }

  pte_t* slot = (pde_t *)HIGHER_HALF(pa) + L2_INDEX(virt);
  pmap_write_pte(space, slot, new_pte, virt);
}

void arch_map_1g(vm_aspace_t* space, uintptr_t virt, uintptr_t phys, uint64_t flags) {
  pde_t *pdep;
  uintptr_t pa = get_space(space, virt);
  pte_t new_pte = make_pte(phys, vm_prot_map[flags & VM_PROT_MASK], flags & VM_CACHE_MASK);

  // Remove L3_PAGE bit -> becomes L1_BLOCK
  new_pte &= ~(1 << 1);

  /* Level 0 */
  pdep = (pde_t *)HIGHER_HALF(pa) + L0_INDEX(virt);
  if (!(pa = PTE_FRAME_ADDR(*pdep))) {
    pa = (uintptr_t)vm_phys_alloc(1, VM_PAGE_ZERO);
    *pdep = pa | L0_TABLE;
  }

  pte_t* slot = (pde_t *)HIGHER_HALF(pa) + L1_INDEX(virt);
  pmap_write_pte(space, slot, new_pte, virt);
}

int pmap_emulate_bits(vm_aspace_t *pmap, uintptr_t va, int prot) {
  uintptr_t pa;

    pte_t pte = *pmap_lookup_pte(pmap, va);

    if ((prot & VM_PROT_READ) && !(pte & ATTR_SW_READ))
      return -1;

    if ((prot & VM_PROT_WRITE) && !(pte & ATTR_SW_WRITE))
      return -2;

    if ((prot & VM_PROT_EXEC) && (pte & ATTR_SW_NOEXEC))
      return -3;

  return 15;
}

/* ================================== *
 *           Global VM API            *
 * ================================== */

uintptr_t virt2phys(vm_aspace_t* sp, uintptr_t virt) {
  pte_t* raw = pmap_lookup_pte(sp, virt);
  if (raw == NULL) {
    return 0;
  } else {
    return PTE_FRAME_ADDR(*raw) | PAGE_OFFSET(virt);
  }
}

void vm_virt_unmap(vm_aspace_t* space, uintptr_t va)  {
  pte_t* pt = pmap_lookup_pte(space, va);
  *pt &= ~(1 << 0);
  __invpg(va, ((va >> 63) & 1) ? 0 : space->asid);
}

/*
void pmap_test() {
  vm_aspace_t kl = {
    .asid = 0,
  };

  asm volatile ("mrs %0, ttbr1_el1" : "=r" (kernel_root));
  uint64_t ip;
  asm volatile ("mov %0, lr" : "=r" (ip));
  /*log("ip -> 0x%p\n", ip);
  kl.root_addr = kernel_root;
  pte_t *pt = pmap_lookup_pte(&kl, ip);
  arch_map_4k(&kl, PTE_FRAME_ADDR(*pt), ip, VM_PROT_READ);
  log("vm: 0x%p -> 0x%p\n", ip, virt2phys(&kl, ip));
  pte_t *pt = pmap_lookup_pte(&kl, ip);
  log("pt -> 0x%p\n", *pt);
}
*/
