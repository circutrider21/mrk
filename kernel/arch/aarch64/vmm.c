#include <vm/virtual.h>
#include <vm/phys.h>
#include <lib/bitops.h>

#define TLBI_ENCODE(a, b) (((uint64_t)a << 48) | (b >> 12))
#define DECODE_ADDR(addr, level) ((addr >> level) & 0x1ff)

#define PAGE_TABLE        (1 << 1)
#define PAGE_L3           (1 << 1)
#define PAGE_ACCESS       (1 << 10)
#define PAGE_INNER_SH     (3 << 8)
#define PAGE_OUTER_SH     (2 << 8)
#define PAGE_WB           (0 << 2)
#define PAGE_GRE          (1 << 2)
#define PAGE_nGnRnE       (2 << 2)
#define PAGE_nGnRE        (3 << 2)
#define PAGE_UC           (4 << 2)

void arch_invl_addr(const void* addr, uint32_t spid) {
    asm volatile (
            "dsb st;\n\t\
		    tlbi vale1, %0;\n\t\
	        dsb sy; isb"
		   :
		   : "r"(TLBI_ENCODE(spid, (uint64_t)addr))
		   : "memory");
}

void arch_invl_spid(uint32_t spid) {
    asm volatile (
           "dsb st;\n\t\
		   tlbi aside1, %0;\n\t\
		   dsb sy; isb"
		   :
		   : "r"(TLBI_ENCODE(spid, 0))
		   : "memory");
}

static uint64_t* next_level(uint64_t* table, int spot) {
    if (!(table[spot] & 1)) {
        uint64_t* sp = (uint64_t*)vm_phys_alloc(1, VM_PAGE_ZERO);
	    table[spot] = (uintptr_t)sp | (uint64_t)1 | PAGE_TABLE;
	    return sp;
    } else {
        return (uint64_t*)(table[spot] & 0xFFFFFFFFF000);
    }
}

void arch_map_4k(vm_aspace_t *spc, uintptr_t phys, uintptr_t virt, uint64_t flags, cache_type ca) {
    // Make a few safety checks
    if (!((virt % 0x1000) + (phys % 0x1000) == 0 && (((virt >> 63) & 1) == 1)))
        return;
    
    int level0_index = DECODE_ADDR(virt, 39);
    int level1_index = DECODE_ADDR(virt, 30);
    int level2_index = DECODE_ADDR(virt, 21);
    int level3_index = DECODE_ADDR(virt, 12);

    uint64_t* level0 = (uint64_t*)(spc->root & 0xFFFFFFFFFFFE);

    // Check for kernel mapping
    if(virt >= 0xFFFFFFFF80000000 || virt >= 0xffff800000000000)
        level0 = (uint64_t*)(spc->kernel_root & 0xFFFFFFFFFFFE);
    
    uint64_t* level1 = next_level(level0, level0_index);
    uint64_t* level2 = next_level(level1, level1_index);
    uint64_t* level3 = next_level(level2, level2_index);

    uint64_t new_entry = phys | 1 | PAGE_L3 | PAGE_ACCESS;
    new_entry |= flags;

    if (ca == CACHE_WRITE_COMBINE)
        new_entry |= PAGE_UC | PAGE_OUTER_SH;
    else if (ca == CACHE_NOCACHE)
	new_entry |= PAGE_nGnRnE | PAGE_OUTER_SH;
    else if (ca == CACHE_MMIO)
	new_entry |= PAGE_nGnRE | PAGE_OUTER_SH;
    else
        new_entry |= PAGE_WB | PAGE_INNER_SH;

    level3[level3_index] = new_entry;
}

uint64_t arch_translate_virt(vm_aspace_t* spc, uintptr_t virt) {
    int level0_index = DECODE_ADDR(virt, 39);
    int level1_index = DECODE_ADDR(virt, 30);
    int level2_index = DECODE_ADDR(virt, 21);
    int level3_index = DECODE_ADDR(virt, 12);

    // TODO: Tell next_level to not create page tables (since we're indexing only and not mapping)
    uint64_t* level0 = (uint64_t*)(spc->root & 0xFFFFFFFFFFFE);

    if(virt >= 0xFFFFFFFF80000000 || virt >= 0xffff800000000000)
        level0 = (uint64_t*)(spc->kernel_root & 0xFFFFFFFFFFFE);
    
    uint64_t* level1 = next_level(level0, level0_index);
    uint64_t* level2 = next_level(level1, level1_index);
    uint64_t* level3 = next_level(level2, level2_index);

    return BITS_READ(level3[level3_index], 12, 35);
}

void arch_swap_table(vm_aspace_t* space) {
    // Only swap ttbr0 since ttbr1 always points to the same page space
    asm volatile ("dsb st; msr ttbr0_el1, %0; dsb sy; isb" :: "r" (((uint64_t)space->spid << 48) | space->root) : "memory");
}
