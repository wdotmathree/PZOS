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
