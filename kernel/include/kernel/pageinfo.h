#pragma once
#ifndef KERNEL_PAGEINFO_H
#define KERNEL_PAGEINFO_H

#include <stdatomic.h>
#include <stdint.h>

#include <kernel/paging.h>
#include <kernel/vmem.h>

enum pageinfo_flags : uint64_t {
	PAGEINFO_NONE = 0x0,
	PAGEINFO_ALLOCATED = 0x1,
	PAGEINFO_SLAB = 0x2,
	PAGEINFO_RESERVED = 0x4,
};

typedef struct pageinfo {
	enum pageinfo_flags flags;

	union {
		struct {
			void *slab_owner;
		};

		struct {
			uint8_t __padding[24];
		};
	};
} pageinfo_t;

static inline pageinfo_t *get_pageinfo(const physaddr_t phys) {
	return (pageinfo_t *)VMEM_PAGEINFO_BASE + (phys / PAGE_SIZE);
}

#endif
