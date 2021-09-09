#ifndef LIB_BUILTINS_H
#define LIB_BUILTINS_H

#include <stddef.h>

#define ALIGN_DOWN(__addr, __align) ((__addr) & ~((__align)-1))
#define ALIGN_UP(__addr, __align) (((__addr) + (__align)-1) & ~((__align)-1))

size_t strlen(const char *str);
void *memcpy(void *dest, const void *src, size_t n);

#endif // LIB_BUILTINS_H

