#include <vm/virtual.h>
#include <vm/phys.h>
#include <generic/log.h>

#include <stdbool.h>

#ifdef __aarch64__  
static int do_flag_swap(int flags) {
    int rflags = 0;
    bool read = flags & VM_PERM_READ;
    bool write = flags & VM_PERM_WRITE;
    bool exec = flags & VM_PERM_EXEC;

    // By default, aarch64 mapping are rwx (Readable Writable and Executable)
    if (read && !write)
        rflags |= VM_PAGE_READONLY;
    
    if (write && !read)
        log("vm/virt: Write Only mappings are not available on aarch64\n");
    
    if (!exec)
        rflags |= VM_PAGE_NOEXEC;

    return rflags;
}
#endif

void vm_virt_map(vm_aspace_t* space, uintptr_t virt, uintptr_t phys, int flags, cache_type ctype) {
    int real_flags = do_flag_swap(flags);
    arch_map_4k(space, phys, virt, real_flags, ctype);
}

void vm_virt_setup(vm_aspace_t* space) {
    space->root = (uintptr_t)vm_phys_alloc(1, VM_PAGE_ZERO);
    space->spid = 0;
}

