#include <lib/bitops.h>
#include <lib/builtins.h>

void bitmap_clear(bitmap_t *bitmap, size_t index) {
    uint64_t bit = index % 8;
    uint64_t byte = index / 8;

    bitmap->map[byte] &= ~(1 << bit);
}

void bitmap_set(bitmap_t *bitmap, size_t index) {
    uint64_t bit = index % 8;
    uint64_t byte = index / 8;
    
    bitmap->map[byte] |= (1 << bit);
}

int bitmap_get(bitmap_t *bitmap, size_t index) {
    uint64_t bit = index % 8;
    uint64_t byte = index / 8;
    
    return bitmap->map[byte] & (1 << bit);
}

void bitmap_fill(bitmap_t *bitmap, int val) {
    if (val) {
        memset(bitmap->map, 0xff, bitmap->size);
    } else {
	memset(bitmap->map, 0, bitmap->size);
    }
}

