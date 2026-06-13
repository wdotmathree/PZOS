#pragma once
#ifndef KERNEL_MMAN_H
#define KERNEL_MMAN_H

#include <limine.h>
#include <stddef.h>

#include <kernel/paging.h>

void mman_init(struct limine_memmap_response *mmap, uint8_t **framebuf, uintptr_t hhdm_off, uintptr_t kernel_end);

physaddr_t alloc_page(void);
void free_page(void *ptr);

// Allocates `npages` pages of memory aligned to `align` pages
physaddr_t alloc_aligned(size_t npages, size_t align);
physaddr_t alloc_contig(size_t npages);
void free_contig(void *ptr, size_t npages);

#endif
