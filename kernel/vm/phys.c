#include <vm/phys.h>
#include <vm/vm.h>
#include <lib/bitops.h>

bitmap_t map;
uintptr_t highest_addr;
uint64_t memstats[2]; // Used - 0 | Available - 1

static void mark_free(void *addr) {
    bitmap_clear(&map, (size_t)addr / VM_PAGE_SIZE);
    
    memstats[0] -= 4096;
    memstats[1] += 4096;   
}

static void mark_used(void *addr) {
    bitmap_set(&map, (size_t)addr / VM_PAGE_SIZE);

    memstats[0] += 4096;
    memstats[1] -= 4096;    
}

void mark_pages(void *addr, size_t page_count) {
    size_t i;
    for (i = 0; i < page_count; i++) {
        mark_used((void*)(addr + (i * VM_PAGE_SIZE)));
    }
}

void vm_phys_free(void *addr, size_t pages) {
    size_t i;
    for (i = 0; i < pages; i++) {
        mark_free((void*)(addr + (i * VM_PAGE_SIZE)));
    }
}

void* vm_phys_alloc(size_t pages, int flags) {
    if (!(pages > 0 && memstats[1] > 0 && pages < map.size))
        return NULL;	    

    size_t i, j;
    void* ret = NULL;
    for (i = 0; i < highest_addr / VM_PAGE_SIZE; i++) {
        for (j = 0; j < pages; j++) {
	    if (bitmap_get(&map, i)) {
	        break;
	    } else if (j == pages - 1) {
	        ret = (void*)(i * VM_PAGE_SIZE);
		    mark_pages(ret, pages);
            goto out;
	    }
	}
    }

out:
    if(flags & VM_PAGE_ZERO && ret != NULL)
        memset(ret, 0, pages * VM_PAGE_SIZE);

    if (ret == NULL) {
        log("vm/phys: Out Of Memory!\n"); 
        return NULL;
    } else {
        return ret;
    }
}

void vm_phys_lock(void* addr, size_t pages) {
    mark_pages(addr, pages);
}

