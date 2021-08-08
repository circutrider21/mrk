#include <cstddef>
#include <internal/lock.h>
#include <mrk/pmm.h>
#include <mrk/vmm.h>

static mutex alloc_lock;

extern "C" {

int liballoc_lock()
{
    alloc_lock.lock();
    return 0;
}

int liballoc_unlock()
{
    alloc_lock.unlock();
    return 0;
}

void* liballoc_alloc(int pages)
{
    uint64_t pgs = (uint64_t)mm::pmm::get(pages);
    void* to_ret = (void*)(pgs + MEM_VIRT_OFFSET);

    for (int i = 0; i < pages; i++) {
        mm::vmm::kernel_space()->map(pgs, pgs + MEM_VIRT_OFFSET, PAGE_PRESENT | PAGE_WRITEABLE | PAGE_NX | PAGE_GLOBAL);
        pgs += 0x1000;
    }

    return to_ret;
}

int liballoc_free(void* ptr, int pages)
{
    mm::pmm::free((void*)((uint64_t)ptr - MEM_VIRT_OFFSET), pages);

    uint64_t pgs = (uint64_t)ptr;
    for (int i = 0; i < pages; i++) {
        mm::vmm::kernel_space()->unmap(pgs);
        pgs += 0x1000;
    }

    return 0;
}
}
