#include <vm/vm.h>
#include <vm/virtual.h>
#include <vm/phys.h>
#include <arch/arch.h>
#include <lib/stivale2.h>
#include <lib/builtins.h>
#include <lib/bitops.h>
#include <generic/log.h>

extern bitmap_t map;
extern uintptr_t highest_addr;
vm_aspace_t root_space;

static void bringup_pmm(struct stivale2_struct_tag_memmap* memory) {
    uintptr_t top;
    size_t i, j, k;

    log("vm/phys: Dumping Memory Map Entries...\n");
    for (i = 0; i < memory->entries; i++) {
        struct stivale2_mmap_entry *entry = &memory->memmap[i];

        log("   +%d [base=0x%x length=%u]\n", i, entry->base, entry->length);

        if (entry->type != STIVALE2_MMAP_USABLE &&
            entry->type != STIVALE2_MMAP_BOOTLOADER_RECLAIMABLE &&
            entry->type != STIVALE2_MMAP_KERNEL_AND_MODULES) {
            continue;
        }

        top = entry->base + entry->length;

        if (top > highest_addr) {
            highest_addr = top;
        }
    }

    size_t bitmap_size = ALIGN_UP(ALIGN_DOWN(highest_addr, VM_PAGE_SIZE) / VM_PAGE_SIZE / 8, VM_PAGE_SIZE);

    map.size = bitmap_size;

    log("vm/phys: bitmap_size is %uKB long\n", map.size / 1024);

    for (j = 0; j < memory->entries; j++) {
        struct stivale2_mmap_entry *entry = &memory->memmap[j];

        if (entry->type == STIVALE2_MMAP_USABLE && entry->length >= bitmap_size) {
            map.map = (uint8_t*)(entry->base + VM_MEM_OFFSET);
            entry->base += bitmap_size;
            entry->length -= bitmap_size;
            break;
        }
    }

    bitmap_fill(&map, 0xff);

    for (k = 0; k < memory->entries; k++) {
        /* If the current entry is usable, set it free in the bitmap */
        if (memory->memmap[k].type == STIVALE2_MMAP_USABLE) {
            vm_phys_free((void*)memory->memmap[k].base, memory->memmap[k].length / VM_PAGE_SIZE);
        }
    }
}

#ifdef __aarch64__
extern struct stivale2_struct_tag_mmio32_uart* mmio_struct;
#endif

static void aarch64_mmu_setup() {
    vm_virt_setup(&root_space);

    // Use ttbr1 from the bootloader (much easier)
    vm_phys_free((void*)root_space.kernel_root, 1);
    root_space.kernel_root = ARM64_READ_REG(ttbr1_el1);

    // Load ttbr0
    uint64_t root_val = ((uint64_t)root_space.spid << 48) | root_space.root;
    ARM64_WRITE_REG(ttbr0_el1, root_val);

    // Sync TLB Changes
    asm volatile ("isb; dsb sy; isb" ::: "memory");
}

void vm_init() {
    struct stivale2_struct_tag_memmap *memory_map = (struct stivale2_struct_tag_memmap*)stivale2_query(STIVALE2_STRUCT_TAG_MEMMAP_ID);
    bringup_pmm(memory_map);

    aarch64_mmu_setup();
    log("vm: Complete!\n");
}

