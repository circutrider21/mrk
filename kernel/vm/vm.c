#include <vm/vm.h>
#include <lib/stivale2.h>
#include <lib/builtins.h>
#include <lib/bitops.h>
#include <generic/log.h>

extern bitmap_t map;
extern uintptr_t highest_addr;

static void bringup_pmm(struct stivale2_struct_tag_memmap* memory) {
    uintptr_t top;
    size_t i, j, k;

    for (i = 0; i < memory->entries; i++) {
        struct stivale2_mmap_entry *entry = &memory->memmap[i];

        log("vm/phys: Entry %d - [base=0x%x length=%u]\n", i, entry->base, entry->length);

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

    // assert_truth(bitmap.size > 0);

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

void vm_init() {
    struct stivale2_struct_tag_memmap *memory_map = stivale2_query(STIVALE2_STRUCT_TAG_MEMMAP_ID);
    bringup_pmm(memory_map);
}

