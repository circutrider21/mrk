#ifndef VM_PHYS_H
#define VM_PHYS_H

#include <stddef.h>

#define VM_PAGE_ZERO  (1 << 0)
#define VM_PAGE_LOCAL (1 << 1) // Not yet used since numa support is yet to be added

void vm_phys_lock(void* addr, size_t pages);
void* vm_phys_alloc(size_t pages, int flags);
void vm_phys_free(void* addr, size_t pages);

#endif // VM_PHYS_H

