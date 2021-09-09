#ifndef LIB_BITOPS_H
#define LIB_BITOPS_H

#include <stdint.h>
#include <stddef.h>

#define BIT_SET(num, pos)   (num |= (1 << pos))
#define BIT_CLEAR(num, pos) (num &= ~(1 << pos))
#define BIT_TEST(num, pos)  (num & (1 << pos))

typedef struct {
    uint8_t* map;
    size_t size;
} bitmap_t;

void bitmap_clear(bitmap_t *bitmap, size_t index);

void bitmap_set(bitmap_t *bitmap, size_t index);

int bitmap_get(bitmap_t *bitmap, size_t index);

void bitmap_fill(bitmap_t *bitmap, int val);

#endif // LIB_BITOPS_H

