#pragma once
#ifndef KERNEL_SLAB_H
#define KERNEL_SLAB_H

#include <stddef.h>
#include <string.h>

#include <kernel/spinlock.h>

typedef struct slabinfo {
	size_t obj_size;
	void *free_list;
	spinlock_t lock;
	char *name;
} slabinfo_t;

slabinfo_t *slab_create(size_t obj_size, const char *name);
// void slab_destroy(slabinfo_t *slab);

void *slab_alloc(slabinfo_t *slab);
void *slab_zalloc(slabinfo_t *slab);
void slab_free(slabinfo_t *slab, void *obj);
void slab_free_unknown(void *obj);

/// TODO: Reclaim all reclaimable pages in the slab back to the system
// void slab_reclaim(slabinfo_t *slab);

#endif
