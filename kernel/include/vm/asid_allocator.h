#ifndef VM_ASID_ALLOCATOR_H
#define VM_ASID_ALLOCATOR_H

#include "stdint.h"

/* A quick note about ASIDs
 *   - 16-bit ASIDs are used on aarch64 and 12-bit ASIDs on x86_64
 *   - The first 10 ASIDs are reserved by the kernel, and are always marked as allocated
 *   - ASID #2 is used as a backup ASID when all others run out, this means that when all ASIDs are
 *     unavailable, the allocator gives out ASID #2.
 */

uint16_t vm_asid_alloc();
void vm_asid_free(uint16_t asid);
void setup_asid();

#endif // VM_ASID_ALLOCATOR_H
