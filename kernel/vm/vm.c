#include <vm/vm.h>
#include <vm/virt.h>
#include <vm/phys.h>
#include <vm/asid_allocator.h>
#include <lib/stivale2.h>
#include <lib/builtins.h>
#include <lib/bitops.h>
#include <generic/log.h>

extern bitmap_t map;
extern uintptr_t highest_addr;

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
// Defined in linker script
extern char __ktext_begin[];
extern char __ktext_end[];
extern char __krdata_begin[];
extern char __krdata_end[];
extern char __kdata_begin[];
extern char __kdata_end[];
extern char __kbss_begin[];
extern char __kbss_end[];

// Defined in kernel/arch/aarch64/vmm.c
extern uint64_t kernel_root;
#endif

static vm_aspace_t root_space;

static void map_krange(uint64_t begin, uint64_t end, int flags) {
  uint64_t delta = end - begin;
  for(uint64_t i = 0; i < VM_BYTES_TO_PAGES(delta) * 0x1000; i += 0x1000) {
    uint64_t ph = begin + i - VM_KERNEL_BASE;
    uint64_t vh = begin + i;
    log("p -> 0x%p, v -> 0x%p\n", ph, vh);
    vm_virt_map(&root_space, ph, vh, flags);
  }
}

void aarch64_mmu_setup() {
  // Load ttbr1 into kernel_root (since we're going to use Sabaton's pagemap)
  asm volatile ("mrs %0, ttbr1_el1" : "=r" (kernel_root));

  root_space.asid = vm_asid_alloc();
  root_space.root_addr = (uintptr_t)vm_phys_alloc(1, VM_PAGE_ZERO);

  log("phys: 0x%p -> virt: 0x%p\n", __ktext_begin - VM_KERNEL_BASE, __ktext_begin);

  // Map the kernel properly
  map_krange((uint64_t)__ktext_begin, (uint64_t)__ktext_end, VM_PROT_READ | VM_PROT_EXEC);
  map_krange((uint64_t)__krdata_begin, (uint64_t)__krdata_end, VM_PROT_READ);
  map_krange((uint64_t)__kdata_begin, (uint64_t)__kdata_end, VM_PROT_READ | VM_PROT_WRITE);
  map_krange((uint64_t)__kbss_begin, (uint64_t)__kbss_end, VM_PROT_READ | VM_PROT_WRITE);

  log("res-> %d\n", pmap_emulate_bits(&root_space, __ktext_begin, VM_PROT_READ | VM_PROT_EXEC));
}

void vm_init() {
    struct stivale2_struct_tag_memmap *memory_map = (struct stivale2_struct_tag_memmap*)stivale2_query(STIVALE2_STRUCT_TAG_MEMMAP_ID);
    bringup_pmm(memory_map);

    setup_asid();
    aarch64_mmu_setup();
    // pmap_test();
}

