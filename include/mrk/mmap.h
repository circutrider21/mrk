#pragma once

#include <mrk/vmm.h>

#define MAP_FAILED -1
#define MAP_PRIVATE 0x1
#define MAP_SHARED 0x2
#define MAP_FIXED 0x4
#define MAP_ANONYMOUS 0x8

#define PROT_NONE 0x00
#define PROT_READ 0x01
#define PROT_WRITE 0x02
#define PROT_EXEC 0x04

namespace mm {
void mmap(mm::vmm::aspace* space, uint64_t sz, void* virt, void* phys, int prot, int flags);
void munmap(mm::vmm::aspace* space, void* addr, uint64_t len);
}
