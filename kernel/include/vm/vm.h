#ifndef VM_H
#define VM_H

/* A quick thing about the following constants...
 * For mrk to be ported to any arch, it must support paging and 48-bit address widths.
 * The following constants are mandatory and CANNOT BE CHANGED. */

#define VM_PAGE_SIZE   0x1000 // aka 4096 or 4KB
#define VM_MEM_OFFSET  ((uintptr_t)0xffff800000000000)
#define VM_KERNEL_BASE ((uintptr_t)0xffffffff80000000)


#define VM_BYTES_TO_PAGES(num) (((num) + 4096 - 1) / 4096)
void vm_init();

#endif // VM_H

