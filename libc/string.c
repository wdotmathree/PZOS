#include <string.h>

#include <stdint.h>

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

void *memcpy(void *restrict dst, const void *restrict src, size_t n) {
	char *d = dst;
	const char *s = src;
	while (n >= 16) {
		asm("movups xmm0, [%0]\n\t"
			"movups [%1], xmm0"
			: : "r"(s), "r"(d) : "xmm0", "memory");
		d += 16;
		s += 16;
		n -= 16;
	}
	while (n >= 8) {
		*(size_t *)d = *(const size_t *)s;
		n -= 8;
		d += 8;
		s += 8;
	}
	while (n--)
		*d++ = *s++;
	return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
	if (dst < src)
		return memcpy(dst, src, n);

	char *d = dst;
	const char *s = src;
	const char *lasts = s + (n - 8);
	char *lastd = d + (n - 8);
	int rem = n % 8;
	n -= rem;
	while (rem--)
		*d++ = *s++;
	while (n >= 8) {
		*(size_t *)lastd = *(size_t *)lasts;
		n -= 8;
		lastd -= 8;
		lasts -= 8;
	}
	return dst;
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
