#include <vm/virtual.h>
#include <vm/phys.h>
#include <vm/vm.h>
#include <lib/bitops.h>
#include <generic/log.h>
#include <arch/arch.h>
#include <stdbool.h>

#define TLBI_ENCODE(a, b) (((uint64_t)a << 48) | (b >> 12))
#define DECODE_ADDR(addr, level) ((addr >> level) & 0x1ff)
#define HIGHER_POINTER(ptr) ((uint64_t*)((uint64_t)ptr + VM_MEM_OFFSET))

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
            "dsb ishst;\n\t\
		     tlbi vae1is, %0;\n\t\
	         dsb ish; isb"
		   :
		   : "r"(TLBI_ENCODE(spid, (uint64_t)addr))
		   : "memory");
}

void arch_invl_spid(uint32_t spid) {
    asm volatile (
            "dsb ishst;\n\t\
		     tlbi aside1is, %0;\n\t\
	         dsb ish; isb"
		   :
		   : "r"((uint64_t)(spid) << 48)
		   : "memory");
}

static uint64_t* page_table_walk(bool create, uint64_t* table, uint16_t index) {
    // log("vm/virt: page_table_walk(%s, 0x%p, %u)\n", create ? "true" : "false", table, index);
    uint64_t pte = table[index];
    if (!(pte & 1ull)) {
        if (!create) {
            return NULL;
        }

        uintptr_t table_ptr = (uintptr_t)vm_phys_alloc(1, VM_PAGE_ZERO);

        table[index] = table_ptr | 0x3;
        return (uint64_t*)table_ptr;
    } else {
        return (uint64_t*)(pte & 0xfffffffff000);
    }
}

void arch_map_4k(vm_aspace_t* space, uintptr_t phys, uintptr_t virt, uint64_t flags, cache_type ctype) {
    uint16_t ttbr = (virt >> 63) & 1;
    uint64_t* level0;

    uint16_t level0_index = DECODE_ADDR(virt, 39);
    uint16_t level1_index = DECODE_ADDR(virt, 30);
    uint16_t level2_index = DECODE_ADDR(virt, 21);
    uint16_t level3_index = DECODE_ADDR(virt, 12);

    if (ttbr == 1) {
        level0 = (uint64_t*)(space->kroot);
    } else {
        level0 = (uint64_t*)(space->root);
    }

    // log("vm: 0x%p 0x%p\n", level0, (uint64_t)level0 + VM_MEM_OFFSET);
    uint64_t* level1 = page_table_walk(true, (uint64_t*)((uint64_t)level0 + VM_MEM_OFFSET), level0_index);
    uint64_t* level2 = page_table_walk(true, (uint64_t*)((uint64_t)level1 + VM_MEM_OFFSET), level1_index);
    uint64_t* level3 = page_table_walk(true, (uint64_t*)((uint64_t)level2 + VM_MEM_OFFSET), level2_index);
    
    uint64_t new_entry = phys | flags | 0x3 | (1 << 10);

    if (ctype == CACHE_STANDARD) {
        new_entry |= (2 << 2);
    }

    HIGHER_POINTER(level3)[level3_index] = new_entry;
}

void arch_map_2m(vm_aspace_t* space, uintptr_t phys, uintptr_t virt, uint64_t flags, cache_type ctype) {
    uint16_t ttbr = (virt >> 63) & 1;
    uint64_t* level0;

    uint16_t level0_index = DECODE_ADDR(virt, 39);
    uint16_t level1_index = DECODE_ADDR(virt, 30);
    uint16_t level2_index = DECODE_ADDR(virt, 21);

    if (ttbr == 1) {
        level0 = (uint64_t*)(ARM64_READ_REG(ttbr1_el1));
    } else {
        level0 = (uint64_t*)(space->root);
    }

    uint64_t* level1 = page_table_walk(true, (uint64_t*)((uint64_t)level0 + VM_MEM_OFFSET), level0_index);
    uint64_t* level2 = page_table_walk(true, (uint64_t*)((uint64_t)level1 + VM_MEM_OFFSET), level1_index);  
    uint64_t new_entry = phys | flags | 0x1 | (1 << 10);

    if (ctype == CACHE_STANDARD) {
        new_entry |= (2 << 2);
    }

    HIGHER_POINTER(level2)[level2_index] = new_entry;
}

void arch_map_1g(vm_aspace_t* space, uintptr_t phys, uintptr_t virt, uint64_t flags, cache_type ctype) {
    uint16_t ttbr = (virt >> 63) & 1;
    uint64_t* level0;

    uint16_t level0_index = DECODE_ADDR(virt, 39);
    uint16_t level1_index = DECODE_ADDR(virt, 30);

    if (ttbr == 1) {
        level0 = (uint64_t*)(ARM64_READ_REG(ttbr1_el1));
    } else {
        level0 = (uint64_t*)(space->root);
    }

    uint64_t* level1 = page_table_walk(true, (uint64_t*)((uint64_t)level0 + VM_MEM_OFFSET), level0_index);
    uint64_t new_entry = phys | flags | 0x1 | (1 << 10);

    if (ctype == CACHE_STANDARD) {
        new_entry |= (2 << 2);
    }

    HIGHER_POINTER(level1)[level1_index] = new_entry;
}

void vm_virt_unmap(vm_aspace_t* space, uintptr_t virt) {
    uint16_t ttbr = (virt >> 63) & 1;
    uint64_t* level0;

    uint16_t level0_index = DECODE_ADDR(virt, 39);
    uint16_t level1_index = DECODE_ADDR(virt, 30);
    uint16_t level2_index = DECODE_ADDR(virt, 21);
    uint16_t level3_index = DECODE_ADDR(virt, 12);

    if (ttbr == 1) {
        level0 = (uint64_t*)(space->root);
    } else {
        level0 = (uint64_t*)(space->root);
    }

    uint64_t* level1 = page_table_walk(false, (uint64_t*)((uint64_t)level0 + VM_MEM_OFFSET), level0_index);
    if (level1 == NULL)
        return;
    else if (!(HIGHER_POINTER(level1)[level1_index] & 1))
        return;
    else if (!(HIGHER_POINTER(level1)[level1_index] & 0x3)) {
        // 1GB mapping, invalidate it
        HIGHER_POINTER(level1)[level1_index] ^= 1;
    }
    
    uint64_t* level2 = page_table_walk(false, (uint64_t*)((uint64_t)level1 + VM_MEM_OFFSET), level1_index);
    if (level2 == NULL)
        return;
    else if (!(HIGHER_POINTER(level2)[level2_index] & 1))
        return;
    else if (!(HIGHER_POINTER(level2)[level2_index] & 0x3)) {
        // 2MB mapping, invalidate it
        HIGHER_POINTER(level2)[level2_index] ^= 1;
    }

    uint64_t* level3 = page_table_walk(false, (uint64_t*)((uint64_t)level2 + VM_MEM_OFFSET), level2_index);
    if (level3 == NULL)
        return;
    else if (!(HIGHER_POINTER(level3)[level3_index] & 1))
        return;
    else if (HIGHER_POINTER(level3)[level3_index] & 0x3) {
        // 4KB mapping, invalidate it
        HIGHER_POINTER(level3)[level3_index] ^= 1;
    }

    arch_invl_addr((void*)virt, space->spid);
}

void arch_swap_table(vm_aspace_t* space) {
    uint64_t root_val = ((uint64_t)space->spid << 48) | space->root;
    ARM64_WRITE_REG(ttbr1_el1, ARM64_READ_REG(ttbr1_el1));
    ARM64_WRITE_REG(ttbr0_el1, root_val);

    // Sync TLB Changes
    asm volatile ("isb; dsb sy; isb" ::: "memory");
}
