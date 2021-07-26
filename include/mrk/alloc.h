#pragma once

#include <cstddef>
#include <cstdint>

#define HEAP_MAGIC 0x3DA4
#define ALIGN_DOWN(n, a) (((uint64_t)n) & ~((a)-1ul))
#define ALIGN_UP(n, a) ALIGN_DOWN(((uint64_t)n) + (a)-1ul, (a))

namespace mm {
struct __attribute__((aligned(64))) heap_hdr {
    struct heap_hdr* nxt;
    size_t size;
    bool free;
    uint16_t magic;

    bool check_magic() { return magic == HEAP_MAGIC; }
};

void init_alloc();
void* alloc(size_t size);
void free(void* ptr);
void* ralloc(void* old_ptr, size_t size);
}

__attribute__((always_inline)) inline void* operator new(size_t size) { return mm::alloc(size); }
__attribute__((always_inline)) inline void* operator new[](size_t size) { return mm::alloc(size); }
__attribute__((always_inline)) inline void* operator new([[maybe_unused]] size_t n, void* ptr) { return ptr; };

__attribute__((always_inline)) inline void operator delete(void* obj) { mm::free(obj); }
__attribute__((always_inline)) inline void operator delete(void* obj, size_t) { mm::free(obj); }
