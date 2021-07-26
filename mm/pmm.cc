#include <internal/builtin.h>
#include <internal/stivale2.h>
#include <mrk/arch.h>
#include <mrk/pmm.h>

#include <cstddef>
#include <cstring>
#include <mrk/log.h>

// A Simple bitmap implentation
class memory_bitmap {
public:
    void used(uint64_t addr, uint64_t pages)
    {
        if (map == nullptr)
            return;

        for (uint64_t i = addr; i < addr + (pages * PAGE_SIZE); i += PAGE_SIZE)
            map[i / (PAGE_SIZE * BMP_CAPACITY)] &= ~((1 << ((i / PAGE_SIZE) % BMP_CAPACITY)));
    }
    void set_map(void* mp, uint64_t mz)
    {
        map = (uint8_t*)mp;
        map_size = mz;
        _memset((uint64_t)mp, 0, map_size);
    }
    void* get_ptr() { return (void*)map; }
    bool is_free(uint64_t addr, uint64_t pages)
    {
        if (map == nullptr)
            return false;

        bool status = true;
        for (uint64_t i = addr; i < addr + (pages * PAGE_SIZE); i += PAGE_SIZE) {
            status = map[i / (PAGE_SIZE * BMP_CAPACITY)] & (1 << ((i / PAGE_SIZE) % BMP_CAPACITY));
            if (!status)
                break;
        }
        return status;
    }

private:
    uint8_t* map;
    uint64_t map_size;
};

static memory_bitmap main_map;
static struct stivale2_struct_tag_memmap* map_tag;
static uint64_t memstats[4]; // 0 - total | 1 - used | 2 - avail | 3 - limit

namespace mm::pmm {
void init()
{
    map_tag = (stivale2_struct_tag_memmap*)arch::stivale2_get_tag(STIVALE2_STRUCT_TAG_MEMMAP_ID);
    if (map_tag == nullptr)
        log("pmm: ERROR!\n");

    // First off, calculate memory stats
    for (size_t i = 0; i < map_tag->entries; i++) {
        struct stivale2_mmap_entry entry = map_tag->memmap[i];
        if (entry.base + entry.length <= 0x100000)
            continue;

        uint64_t new_limit = entry.base + entry.length;
        if (new_limit > memstats[3])
            memstats[3] = new_limit;

        if (entry.type == STIVALE2_MMAP_USABLE || entry.type == STIVALE2_MMAP_BOOTLOADER_RECLAIMABLE || entry.type == STIVALE2_MMAP_ACPI_RECLAIMABLE || entry.type == STIVALE2_MMAP_KERNEL_AND_MODULES)
            memstats[0] += entry.length;
    }

    // Then find a nice spot for our bitmap
    uint64_t bitmap_size = memstats[3] / (PAGE_SIZE * BMP_CAPACITY);
    for (size_t i = 0; i < map_tag->entries; i++) {
        struct stivale2_mmap_entry entry = map_tag->memmap[i];
        if (entry.base + entry.length <= 0x100000)
            continue;

        if (entry.length >= bitmap_size && entry.type == STIVALE2_MMAP_USABLE) {
            main_map.set_map((void*)((uint64_t)entry.base + MEM_VIRT_OFFSET), bitmap_size);
            break;
        }
    }

    // Finnally, fill up the bitmap
    for (size_t i = 0; i < map_tag->entries; i++) {
        struct stivale2_mmap_entry entry = map_tag->memmap[i];
        if (entry.base + entry.length <= 0x100000)
            continue;

        if (entry.type == STIVALE2_MMAP_USABLE) {
            mm::pmm::free((void*)entry.base, NUM_PAGES(entry.length));
        }
    }

    // Mark the bitmap :-)
    mm::pmm::lock((void*)((uint64_t)main_map.get_ptr() - MEM_VIRT_OFFSET), NUM_PAGES(bitmap_size));
}
void reclaim()
{
    for (size_t i = 0; i < map_tag->entries; i++) {
        struct stivale2_mmap_entry entry = map_tag->memmap[i];

        if (entry.type == STIVALE2_MMAP_BOOTLOADER_RECLAIMABLE)
            mm::pmm::free((void*)entry.base, NUM_PAGES(entry.length));
    }
}

void* get(uint64_t pages)
{
    static uint64_t last_page;
    for (uint64_t i = last_page; i < memstats[3]; i += PAGE_SIZE) {
        if (mm::pmm::lock((void*)i, pages))
            return (void*)i;
    }

    for (uint64_t i = 0; i < last_page; i += PAGE_SIZE) {
        if (mm::pmm::lock((void*)i, pages))
            return (void*)i;
    }

    todo("Add Panic support and panic here (Out of Memory)\n");
    return nullptr;
}

void free(void* addr, uint64_t pages)
{
    for (uint64_t i = (uint64_t)addr; i < (uint64_t)addr + (pages * PAGE_SIZE); i += PAGE_SIZE) {
        if (!main_map.is_free(i, 1))
            memstats[2] += PAGE_SIZE;

        uint8_t* map = (uint8_t*)main_map.get_ptr();
        map[i / (PAGE_SIZE * BMP_CAPACITY)] |= 1 << ((i / PAGE_SIZE) % BMP_CAPACITY);
    }
}

bool lock(void* addr, uint64_t pages)
{
    if (!main_map.is_free((uint64_t)addr, pages))
        return false;

    main_map.used((uint64_t)addr, pages);
    memstats[2] -= pages * PAGE_SIZE;
    return true;
}

void dump()
{
    log("mm/pmm: Dumping Memory Info...\n");

    memstats[1] = memstats[0] - memstats[2];

    log("\tTotal: %d KiB (%d MiB)\n", memstats[0] / 1024, memstats[0] / (1024 * 1024));
    log("\tUsed:  %d KiB (%d MiB)\n", memstats[1] / 1024, memstats[1] / (1024 * 1024));
    log("\tAvailable:  %d KiB (%d MiB)\n", memstats[2] / 1024, memstats[2] / (1024 * 1024));

    log("\tHighest Available Physical Address: %x\n", memstats[3]);
}
}
