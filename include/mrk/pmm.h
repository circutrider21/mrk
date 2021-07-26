#pragma once

#include <cstdint>

#define PAGE_SIZE 4096
#define MEM_VIRT_OFFSET 0xffff800000000000
#define HIGHERHALF_OFFSET 0xffffffff80000000

// How many pages per byte the bitmap can track
#define BMP_CAPACITY 8

// Some page releated functions
#define NUM_PAGES(num) (((num) + PAGE_SIZE - 1) / PAGE_SIZE)
#define PAGE_ALIGN_UP(num) (NUM_PAGES(num) * PAGE_SIZE)
#define V2P(a) (((uint64_t)(a)) - MEM_VIRT_OFFSET)
#define P2V(a) (((uint64_t)(a)) + MEM_VIRT_OFFSET)

namespace mm::pmm {
void init();

// Reclaims bootloader memory
void reclaim();

void* get(uint64_t pages);
void free(void* addr, uint64_t pages);
bool lock(void* addr, uint64_t pages);

// Logs the total/used/avail amounts of memory
void dump();
}
