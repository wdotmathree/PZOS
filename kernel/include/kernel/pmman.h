#pragma once
#ifndef KERNEL_PMMAN_H
#define KERNEL_PMMAN_H

#include <limine.h>
#include <stddef.h>

struct page_block {
	void *addr;
	size_t npages;
	struct page_block *next;
	uint64_t _ign;
};

struct palloc_info {
	size_t rem;
	struct page_block *head;
	struct page_block *tail;
	uint64_t _ign;
};

void pmman_init(struct limine_memmap_response *mmap, intptr_t hhdm_off, intptr_t kernel_base, intptr_t kernel_size);
void kpfree(void *ptr, size_t npages);
struct palloc_info *kpalloc(size_t npages);

#endif
