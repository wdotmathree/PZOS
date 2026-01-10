#pragma once
#ifndef KERNEL_DEFINES_H
#define KERNEL_DEFINES_H

#include_next <defines.h>

#include <stdint.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

typedef uintptr_t physaddr_t;

#endif
