#include <lib/builtins.h>
#include <stdint.h>

size_t strlen(const char *str) {
	size_t result = 0;
	while (str[result] != '\0') {
		result++;
	}
	return result;
}

void *memcpy(void *dest, const void *src, size_t n) {
	char *cdest = dest;
	const char *csrc = src;
	for (size_t i = 0; i < n; ++i) {
		cdest[i] = csrc[i];
	}
	return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i])
            return p1[i] < p2[i] ? -1 : 1;
    }

    return 0;
}

void memset(void* data, const uint8_t value, const size_t n) {
    for(size_t i = 0; i < n; i++)
        ((uint8_t*)data)[i] = value;
}

