#include <string.h>

void *memcpy(void *__restrict dst, const void *__restrict src, size_t n) {
	char *d = dst;
	const char *s = src;
	while (n--)
		*d++ = *s++;
	return dst;
}
