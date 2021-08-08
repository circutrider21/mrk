#include <internal/lock.h>
#include <klib/builtin.h>
#include <mrk/log.h>
#include <mrk/mmap.h>
#include <mrk/pmm.h>

static mutex mmap_mutex;

void mmap(mm::vmm::aspace* space, uint64_t sz, void* virt, void* phys, int prot, int flags)
{
    lock_retainer guard { mmap_mutex };

    if (!(flags & (1 << MAP_ANONYMOUS))) {
        // No support for shared mmap yet
        todo("Add support for MAP_SHARED\n");
        return;
    }

    if (!virt) {
        // Can't yet find free virtual space
        todo("Find free virtual addresses");
        return;
    }

    uint64_t map_flags = ((prot & PROT_READ) ? (PAGE_PRESENT) : 0) | ((prot & PROT_WRITE) ? (PAGE_WRITEABLE) : 0) | (!(prot & PROT_EXEC) ? (PAGE_NX) : 0) | PAGE_USER;
    uint64_t pages = ROUND_UP(sz, PAGE_SIZE);
    uint64_t nvirt = (uint64_t)virt;
    uint64_t nphys = (uint64_t)phys;

    for (uint64_t d = 0; d < pages; d++, nvirt += PAGE_SIZE, nphys += PAGE_SIZE) {
        if (phys == nullptr) {
            void* block = mm::pmm::get(1);
            if (block == nullptr)
                return;
            space->map((uint64_t)block, nvirt, map_flags);
            _memset((uint64_t)virt, 0, 4096);
        } else {
            space->map(nphys, nvirt, map_flags);
        }
    }
}

void munmap(mm::vmm::aspace* space, void* addr, uint64_t len)
{
    lock_retainer guard { mmap_mutex };
    mm::pmm::free(addr, len);

    uint64_t nvirt = (uint64_t)addr;
    for (uint64_t d = 0; d < ROUND_UP(len, PAGE_SIZE); d++, nvirt += PAGE_SIZE) {
        space->unmap(nvirt);
    }
}
