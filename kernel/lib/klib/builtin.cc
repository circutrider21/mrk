#include <klib/builtin.h>
#include <mrk/log.h>

void _memset(uint64_t b, int c, int len)
{
    unsigned char* p = (unsigned char*)b;
    while (len > 0) {
        *p = c;
        p++;
        len--;
    }
    return;
}

int _strcmp(const char* s1, const char* s2)
{
    for (size_t i = 0;; i++) {
        char c1 = s1[i], c2 = s2[i];
        if (c1 != c2)
            return c1 - c2;
        if (!c1)
            return 0;
    }
}

int _memcmp(const void* s1, const void* s2, size_t n)
{
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i])
            return p1[i] < p2[i] ? -1 : 1;
    }

    return 0;
}

size_t _strlen(const char* str)
{
    for (size_t len = 0;; ++len)
        if (str[len] == 0)
            return len;
}

void _memcpy(void* dest, const void* src, size_t count)
{
    const uint8_t* p1 = (const uint8_t*)dest;
    uint8_t* p2 = (uint8_t*)src;

    for (size_t i = 0; i < count; i++)
        p2[i] = p1[i];

    return;
}

char* _strcpy(char* dest, const char* src)
{
    size_t i;

    for (i = 0; src[i]; i++)
        dest[i] = src[i];

    dest[i] = 0;

    return dest;
}
