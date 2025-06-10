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

void *memmove(void *dst, const void *src, size_t len) {
	char *d = dst;
	const char *s = src;
	if (s > d) {
		while (len >= 8) {
			*(size_t *)d = *(const size_t *)s;
			len -= 8;
			d += 8;
			s += 8;
		}
		while (len--)
			*d++ = *s++;
	} else {
		const char *lasts = s + (len - 8);
		char *lastd = d + (len - 8);
		int rem = len % 8;
		len -= rem;
		while (rem--)
			*d++ = *s++;
		while (len >= 8) {
			*(size_t *)lastd = *(size_t *)lasts;
			len -= 8;
			lastd -= 8;
			lasts -= 8;
		}
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
