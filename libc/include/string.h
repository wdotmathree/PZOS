#pragma once
#ifndef STRING_H
#define STRING_H

#include <stddef.h>

int memcmp(const void *lhs, const void *rhs, size_t count);
void *memcpy(void *restrict dest, const void *restrict src, size_t count);
void *memmove(void *dest, const void *src, size_t count);
void *memset(void *dest, int ch, size_t count);
size_t strlen(const char *str);

#endif
