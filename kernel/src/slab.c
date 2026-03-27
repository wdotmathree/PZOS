#include <kernel/slab.h>

#include <kernel/mman.h>
#include <kernel/pageinfo.h>
#include <kernel/panic.h>

// Min object size is calculated as 2^(SLAB_MIN_ORDER+3)
#define SLAB_MIN_ORDER 0

static slabinfo_t global_slab = {
	.obj_size = sizeof(slabinfo_t),
	.free_list = NULL,
	.lock = SPINLOCK_INITIALIZER,
	.name = "global_slab"
};

slabinfo_t *slab_create(size_t obj_size, const char *name) {
	if (obj_size == 0 || obj_size > PAGE_SIZE)
		return NULL;

	slabinfo_t *slab = (slabinfo_t *)slab_alloc(&global_slab);
	if (!slab)
		return NULL;

	if (obj_size < (1ULL << (SLAB_MIN_ORDER + 3))) {
		slab->obj_size = 8;
	} else if (obj_size & (obj_size - 1)) {
		// Round up to the next power of two
		size_t size = 1ULL << (SLAB_MIN_ORDER + 3);
		while (size < obj_size) {
			size <<= 1;
		}
		slab->obj_size = size;
	} else {
		slab->obj_size = obj_size;
	}
	slab->free_list = NULL;
	slab->name = (char *)name;
	spinlock_init(&slab->lock);

	return slab;
}

void *slab_alloc(slabinfo_t *slab) {
	spin_acquire(&slab->lock);

	if (slab->free_list) {
		// Reuse an object from the free list
		void *obj = slab->free_list;
		slab->free_list = *(void **)obj;
		spin_release(&slab->lock);
		return obj;
	}

	// Allocate a new page
	// Set the free list to the second object and link
	slab->free_list = __va(alloc_page()) + slab->obj_size;
	for (uintptr_t offset = slab->obj_size; offset < PAGE_SIZE; offset += slab->obj_size) {
		*(void **)(slab->free_list + offset - slab->obj_size) = slab->free_list + offset;
	}
	*(void **)(slab->free_list + PAGE_SIZE - slab->obj_size * 2) = NULL;

	// Update pageinfo
	pageinfo_t *info = get_pageinfo(__pa(slab->free_list));
	info->flags |= PAGEINFO_SLAB;
	info->slab_owner = slab;

	// Return the first object
	void *obj = slab->free_list - slab->obj_size;

	spin_release(&slab->lock);
	return obj;
}

void *slab_zalloc(slabinfo_t *slab) {
	void *obj = slab_alloc(slab);
	if (obj)
		memset(obj, 0, slab->obj_size);

	return obj;
}

void slab_free2(slabinfo_t *slab, void *obj) {
	spin_acquire(&slab->lock);

	// Add the object back to the free list
	*(void **)obj = slab->free_list;
	slab->free_list = obj;

	spin_release(&slab->lock);
}

void slab_free(void *obj) {
	pageinfo_t *info = get_pageinfo(__pa(obj));
	if (!(info->flags & PAGEINFO_SLAB))
		panic("slab_free: invalid free on non-slab object %p", obj);

	slab_free2(get_pageinfo(__pa(obj))->slab_owner, obj);
}
