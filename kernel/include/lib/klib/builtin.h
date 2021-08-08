#pragma once

#include <cstddef>
#include <cstdint>

void _memset(uint64_t b, int c, int len);
int _strcmp(const char* s1, const char* s2);
int _memcmp(const void* s1, const void* s2, size_t n);

size_t _strlen(const char* str);
void _memcpy(void* dest, const void* src, size_t count);
char* _strcpy(char* dest, const char* src);

#define ROUND_UP(n, d) (((n)-1) / (d) + 1)

#define PANIC(msg, ...) ({   \
    log("PANIC: ");          \
    log(msg, ##__VA_ARGS__); \
    asm volatile("cli");     \
    for (;;) {               \
        asm volatile("hlt"); \
    }                        \
})
