#include <lib/builtins.h>

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

