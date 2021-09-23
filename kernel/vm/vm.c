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

// Defined in linker script
extern char __ktext_begin[];
extern char __ktext_end[];
extern char __krdata_begin[];
extern char __krdata_end[];
extern char __kdata_begin[];
extern char __kdata_end[];
extern char __kbss_begin[];
extern char __kbss_end[];
#endif

static void map_range(uint64_t begin, uint64_t end, int flags) {
    uint64_t delta = end - begin;
    for(uint64_t i = 0; i < VM_BYTES_TO_PAGES(delta); i += 0x1000) {
        vm_virt_map(&root_space, begin + i + VM_KERNEL_BASE, begin + i, flags, CACHE_STANDARD);
    }
}

static void aarch64_mmu_setup(struct stivale2_struct_tag_memmap* mem) {
    vm_virt_setup(&root_space);

    // Map the kernel
    map_range(__ktext_begin, __ktext_end, VM_PERM_READ | VM_PERM_EXEC);
    map_range(__krdata_begin, __krdata_end, VM_PERM_READ);
    map_range(__kdata_begin, __kdata_end, VM_PERM_READ | VM_PERM_WRITE);
    map_range(__kbss_begin, __kbss_end, VM_PERM_READ | VM_PERM_WRITE);

    // Map all of memory
    for (int j = 0; j < mem->entries; j++) {
        struct stivale2_mmap_entry *entry = &mem->memmap[j];

        for(uint64_t i = 0; i < VM_BYTES_TO_PAGES(entry->length); i += 0x1000) {
            vm_virt_map(&root_space, entry->base + i + VM_MEM_OFFSET, entry->base + i, 
                VM_PERM_READ | VM_PERM_WRITE, CACHE_STANDARD);
        }
    }

    // Map the framebuffer
    extern uintptr_t framebuffer;
    extern uint16_t width;
    extern uint16_t pitch;

    for(uint64_t i = 0; i < VM_BYTES_TO_PAGES(width * pitch); i += 0x1000) {
        vm_virt_map(&root_space, framebuffer + i, framebuffer + i - VM_MEM_OFFSET, 
            VM_PERM_READ | VM_PERM_WRITE, CACHE_STANDARD);
    }

    // Load ttbr0/ttbr1
    uint64_t root_val = ((uint64_t)root_space.spid << 48) | root_space.root;
    ARM64_WRITE_REG(ttbr0_el1, root_val);
    ARM64_WRITE_REG(ttbr1_el1, root_space.kroot);

    // Sync TLB Changes
    asm volatile ("isb; dsb sy; isb" ::: "memory");
}

void vm_init() {
    struct stivale2_struct_tag_memmap *memory_map = (struct stivale2_struct_tag_memmap*)stivale2_query(STIVALE2_STRUCT_TAG_MEMMAP_ID);
    bringup_pmm(memory_map);

    aarch64_mmu_setup(memory_map);

    log("vm: Complete!\n");
}

