#pragma once
#ifndef KERNEL_KMALLOC_H
#define KERNEL_KMALLOC_H

#include <stddef.h>

#include <kernel/mman.h>

void kmalloc_init(void);

void *kmalloc(size_t size);

void kfree(void *ptr);

#endif
