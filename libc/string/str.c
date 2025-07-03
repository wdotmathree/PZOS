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
