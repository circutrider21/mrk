#include <vm/virtual.h>
#include <vm/phys.h>
#include <generic/log.h>
#include <arch/arch.h>

#include <stdbool.h>

#ifdef __aarch64__ 
static int do_flag_swap(int flags) {
    int rflags = 0;
    bool read = flags & VM_PERM_READ;
    bool write = flags & VM_PERM_WRITE;
    bool exec = flags & VM_PERM_EXEC;
    bool user = flags & VM_PERM_USER;
    bool global = flags & VM_PERM_GLOBAL;

    // By default, aarch64 mapping are rwxg (Readable, Writable, Executable and Global)
    if (read && !write)
        rflags |= VM_PAGE_READONLY;
    
    if (write && !read)
        log("vm/virt: Write Only mappings are not available on aarch64\n");
    
    if (!exec)
        rflags |= VM_PAGE_NOEXEC;
    
    if (!global)
        rflags |= (1 << 11); // nG or Non-Global
    
    if (user)
        rflags |= VM_PAGE_USER;

    return rflags;
}
#endif

void vm_virt_map(vm_aspace_t* space, uintptr_t virt, uintptr_t phys, int flags, cache_type ctype) {
    int real_flags = do_flag_swap(flags);
    if (flags & VM_MAP_1G) {
        arch_map_1g(space, phys, virt, real_flags, ctype);
    } else if (flags & VM_MAP_2MB) {
        arch_map_2m(space, phys, virt, real_flags, ctype);
    } else {
        arch_map_4k(space, phys, virt, real_flags, ctype);
    }
}

void vm_virt_setup(vm_aspace_t* space) {
    space->root = (uintptr_t)vm_phys_alloc(1, VM_PAGE_ZERO);
    space->spid = 0;
    space->kroot = (uintptr_t)vm_phys_alloc(1, VM_PAGE_ZERO);
}

