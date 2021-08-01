#pragma once

#include <cstddef>
#include <cstdint>

// Define the builtins as assembly where possible
static inline void _memset(uint64_t b, int c, int len)
{
    unsigned char* p = (unsigned char*)b;
    while (len > 0) {
        *p = c;
        p++;
        len--;
    }
    return;
}

static inline int _memcmp(const void* str1, const void* str2, size_t count)
{
  const unsigned char *s1 = (const unsigned char*)str1;
  const unsigned char *s2 = (const unsigned char*)str2;

  while (count-- > 0)
    {
      if (*s1++ != *s2++)
	  return s1[-1] < s2[-1] ? -1 : 1;
    }
  return 0;
}

static inline size_t _strlen(const char* str)
{
    for (size_t len = 0;; ++len)
        if (str[len] == 0)
            return len;
}

#define _strcmp(str1, str2) _memcmp(str1, str2, _strlen(str1))

#define _memcpy(src, dest, count) asm("rep movsd"                                           \
                                      :                                                     \
                                      : "S"((uint64_t)src), "D"((uint64_t)dest), "c"(count) \
                                      : "memory");
#define _strcpy(src, dest) asm("rep movsd"                                                  \
                               :                                                            \
                               : "S"((uint64_t)src), "D"((uint64_t)dest), "c"(_strlen(src)) \
                               : "memory");

#define ROUND_UP(n, d) (((n)-1) / (d) + 1)

#define PANIC(msg, ...) ({   \
    log("PANIC: ");          \
    log(msg, ##__VA_ARGS__); \
    asm volatile("cli");     \
    for (;;) {               \
        asm volatile("hlt"); \
    }                        \
})
