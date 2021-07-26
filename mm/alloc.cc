#include <internal/builtin.h>
#include <mrk/alloc.h>
#include <mrk/lock.h>
#include <mrk/log.h>
#include <mrk/pmm.h>
#include <mrk/vmm.h>

static mutex alloc_lock;
static mm::heap_hdr* head = nullptr;
static mm::heap_hdr* tail = nullptr;

// Some compilers have discarded this function
// Let's make sure that dosen't happen.

[[nodiscard]] static mm::heap_hdr* free_block(size_t sz)
{
    mm::heap_hdr* curr = head;
    while (curr) {
        if (curr->free && (curr->size >= sz))
            return curr;
        curr = curr->nxt;
    }
    return nullptr;
}

static uint64_t current_page_offset = 0xffffffffd0000000;
static uint64_t current_offset = 0xffffffffd0000000;

[[nodiscard]] static uint64_t grow_heap(size_t size, uint64_t align = 8)
{
    current_offset = ALIGN_UP(current_offset, align);
    uint64_t ret = current_offset;
    current_offset += size;

    while (current_page_offset <= current_offset) {
        void* block = mm::pmm::get(1);
        if (block == nullptr)
            return 0;
        mm::vmm::kernel_space()->map((uint64_t)block, current_page_offset, PAGE_PRESENT | PAGE_WRITEABLE | PAGE_GLOBAL | PAGE_NX);
        _memset((uint64_t)current_page_offset, 0, 4096);
        current_page_offset += 4096;
    }

    return ret;
}

[[nodiscard]] void* mm::alloc(size_t size)
{
    if (size == 0)
        return nullptr;

    alloc_lock.lock();

    mm::heap_hdr* header = free_block(size);
    if (header) {
        if (!header->check_magic()) {
            log("alloc: Invalid header magic: allocation size: %u, header ptr: %x\n", size, header);
            alloc_lock.unlock();
            return nullptr;
        }

        header->free = false;
        alloc_lock.unlock();
        return (void*)(header + 1);
    }

    size_t total_size = size + sizeof(mm::heap_hdr);
    uint64_t block = grow_heap(total_size, 64);
    if (block == 0) {
        log("alloc: Failed to allocate block with size: %x\n", total_size);
        alloc_lock.unlock();
        return nullptr;
    }

    header = (mm::heap_hdr*)(block);
    header->size = size;
    header->nxt = nullptr;
    header->magic = HEAP_MAGIC;
    header->free = false;

    if (head == nullptr)
        head = header;
    if (tail)
        tail->nxt = header;
    tail = header;

    alloc_lock.unlock();
    return (void*)(header + 1);
}

void mm::free(void* ptr)
{
    if (!ptr)
        return;
    alloc_lock.lock();
    mm::heap_hdr* header = ((mm::heap_hdr*)ptr - 1);

    if (!header->check_magic()) {
        log("alloc: Invalid header magic: header ptr: %x\n", header);
        alloc_lock.unlock();
        return;
    }

    if (header->free) {
        log("alloc: Tried to double free ptr: %x!\n", ptr);
        alloc_lock.unlock();
        return;
    }

    header->free = true;
    alloc_lock.unlock();
}

void* mm::ralloc(void* old_ptr, size_t size)
{
    if(!old_ptr && size == 0) return nullptr;

    if(size == 0){
	mm::free(old_ptr);
        return nullptr;
    }

    if(!old_ptr) return mm::alloc(size);

    mm::heap_hdr* header = (static_cast<mm::heap_hdr*>(old_ptr) - 1);
    if(header->free){
        return nullptr;
    }

    if(!header->check_magic()){
        log("alloc: Invalid magic for heap_hdr %x\n", header);
        return nullptr;
    }

    if(header->size >= size) return old_ptr;

    void* ret = mm::alloc(size);
    if(ret){
        _memcpy(ret, old_ptr, header->size);
        mm::free(old_ptr);
    }

    return ret;
}

void mm::init_alloc()
{
    // Start small, nothing too big
    for (size_t i = 0; i < 4; i++) {
        mm::vmm::kernel_space()->map((uint64_t)mm::pmm::get(1), current_page_offset, PAGE_PRESENT | PAGE_WRITEABLE | PAGE_GLOBAL | PAGE_NX);
        current_page_offset += 4096;
    }
}
