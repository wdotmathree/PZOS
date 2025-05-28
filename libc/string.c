#include <string.h>

int memcmp(const void *s1, const void *s2, size_t len) {
	while (len--) {
		if (*(unsigned char *)s1 != *(unsigned char *)s2) {
			return *(unsigned char *)s1 - *(unsigned char *)s2;
		}
		s1++;
		s2++;
	}
	return 0;
}

void *memcpy(void *__restrict dst, const void *__restrict src, size_t n) {
	char *d = dst;
	const char *s = src;
	while (n--)
		*d++ = *s++;
	return dst;
}

void *memmove(void *dest, const void *src, size_t len) {
	char *d = dest;
	const char *s = src;
	if (d < s) {
		while (len--) {
			*d++ = *s++;
		}
	} else {
		const char *lasts = s + (len - 1);
		char *lastd = d + (len - 1);
		while (len--) {
			*lastd-- = *lasts--;
		}
	}
	return dest;
}

void *memset(void *s, int c, size_t n) {
	char *p = s;
	while (n--)
		*p++ = c;
	return s;
}

size_t strlen(const char *s) {
	size_t len = 0;
	while (s[len])
		len++;
	return len;
}
