#ifndef LIB_BUILTINS_H
#define LIB_BUILTINS_H

#include <stddef.h>
#include <stdint.h>

#define ALIGN_DOWN(__addr, __align) ((__addr) & ~((__align)-1))
#define ALIGN_UP(__addr, __align) (((__addr) + (__align)-1) & ~((__align)-1))

size_t strlen(const char *str);
void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void memset(void* data, const uint8_t value, const size_t n);

#endif // LIB_BUILTINS_H

