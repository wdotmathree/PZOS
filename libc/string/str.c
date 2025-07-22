#include <string.h>

char *strcpy(char *restrict dest, const char *restrict src) {
	char *d = dest;
	while (*src)
		*d++ = *src++;
	*d = '\0';
	return dest;
}

char *strncpy(char *restrict dest, const char *restrict src, size_t count) {
	char *d = dest;
	while (*src && count--)
		*d++ = *src++;
	if (count)
		*d = '\0';
	return dest;
}

char *strcat(char *restrict dest, const char *restrict src) {
	char *d = dest;
	while (*d)
		d++;
	while (*src)
		*d++ = *src++;
	*d = '\0';
	return dest;
}

char *strncat(char *restrict dest, const char *restrict src, size_t count) {
	char *d = dest;
	while (*d)
		d++;
	while (*src && count--)
		*d++ = *src++;
	*d = '\0';
	return dest;
}

size_t strlen(const char *s) {
	size_t len = 0;
	while (s[len])
		len++;
	return len;
}

int strcmp(const char *s1, const char *s2) {
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
	while (n && *s1 && (*s1 == *s2)) {
		s1++;
		s2++;
		n--;
	}
	if (n == 0)
		return 0;
	return *(unsigned char *)s1 - *(unsigned char *)s2;
}

char *strchr(const char *s, int c) {
	while (*s) {
		if (*s == (char)c)
			return (char *)s;
		s++;
	}
	return NULL;
}
