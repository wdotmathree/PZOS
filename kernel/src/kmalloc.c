#include <kernel/kmalloc.h>

#include <kernel/slab.h>

#define KMALLOC_NUM_SLABS (sizeof(sizes) / sizeof(*sizes))

static const int sizes[] = {
	8,
	16,
	32,
	64,
	128,
	256,
	512,
	1024,
	2048,
	4096,
};

static const char *size_names[] = {
	"kmalloc-8",
	"kmalloc-16",
	"kmalloc-32",
	"kmalloc-64",
	"kmalloc-128",
	"kmalloc-256",
	"kmalloc-512",
	"kmalloc-1k",
	"kmalloc-2k",
	"kmalloc-4k",
};

slabinfo_t *kmalloc_slabs[KMALLOC_NUM_SLABS];

void kmalloc_init(void) {
	for (int i = 0; i < KMALLOC_NUM_SLABS; i++) {
		kmalloc_slabs[i] = slab_create(sizes[i], size_names[i]);
	}
}

void *kmalloc(size_t size) {
	for (int i = 0; i < KMALLOC_NUM_SLABS; i++) {
		if (size <= sizes[i])
			return slab_alloc(kmalloc_slabs[i]);
	}
	return NULL;
}

void kfree(void *ptr) {
	slab_free(ptr);
}
