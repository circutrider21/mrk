#pragma once

#include <cstddef>
#include <cstdint>
#include <klib/alloc.h>

namespace mm {
__attribute__((always_inline)) inline void* alloc(size_t size) { return kmalloc(size); }
__attribute__((always_inline)) inline void free(void* ptr) { kfree(ptr); }
__attribute__((always_inline)) inline void* ralloc(void* old_ptr, size_t size) { return krealloc(old_ptr, size); }
}

__attribute__((always_inline)) inline void* operator new(size_t size) { return kmalloc(size); }
__attribute__((always_inline)) inline void* operator new[](size_t size) { return kmalloc(size); }
__attribute__((always_inline)) inline void* operator new([[maybe_unused]] size_t n, void* ptr) { return ptr; };

__attribute__((always_inline)) inline void operator delete(void* obj) { kfree(obj); }
__attribute__((always_inline)) inline void operator delete(void* obj, size_t) { kfree(obj); }
