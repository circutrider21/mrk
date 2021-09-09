#include <vm/virtual.h>
#include <vm/phys.h>
#include <arch/arch.h>

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
    asm volatile ("dsb st;\n\t\
		   tlbi vale1, %0;\n\t\
	           dsb sy; isb"
		   :
		   : "r"(TLBI_ENCODE(spid, (uint64_t)addr))
		   : "memory");
}

void arch_invl_spid(uint32_t spid) {
    asm volatile ("dsb st;\n\t\
		   tlbi aside1, %0;\n\t\
		   dsb sy; isb"
		   :
		   : "r"(TLBI_ENCODE(spid, 0))
		   : "memory");
}

static uint64_t* next_level(uint64_t* table, int spot) {
    if (!(table[spot] & VM_PAGE_PRESENT)) {
        uint64_t* sp = (uint64_t*)vm_page_get(1, VM_PAGE_ZERO);
	table[spot] = sp | (uint64_t)VM_PAGE_PRESENT | PAGE_TABLE;
	return sp;
    } else {
        return (uint64_t*)table[spot] & 0xFFFFFFFFF000;
    }
}

void arch_map_4k(vm_aspace_t *spc, uintptr_t phys, uintptr_t virt, uint64_t flags, cache_type ca) {
    // Make a few safety checks
    if (!((virt % 0x1000) + (phys % 0x1000) == 0 && ((virt >> 63) & 1 == 1)))
        return;

    int level0_index = DECODE_ADDR(virt, 39);
    int level1_index = DECODE_ADDR(virt, 30);
    int level2_index = DECODE_ADDR(virt, 21);
    int level3_index = DECODE_ADDR(virt, 12);

    uint64_t* level0 = (uint64_t*)spc->root & 0xFFFFFFFFFFFE;
    uint64_t* level1 = next_level(level0, level0_index);
    uint64_t* level2 = next_level(level1, level1_index);
    uint64_t* level3 = next_level(level2, level2_index);

    uint64_t new_entry = phys | VM_PAGE_PRESENT | PAGE_L3 | PAGE_ACCESS;
    new_entry |= flags;

    if (ca == CACHE_WRITE_COMBINE)
        new_entry |= PAGE_UC | PAGE_OUTER_SH;
    else if (ca == CACHE_NOCACHE)
	new_entry |= PAGE_GnRnE | PAGE_OUTER_SH;
    else if (ca == CACHE_MMIO)
	new_entry |= PAGE_nGnRE | PAGE_OUTER_SH;
    else
        new_entry |= PAGE_WB | PAGE_INNER_SH;

    level3[level3_index] = new_entry;
}

