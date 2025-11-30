#include <kernel/kmalloc.h>

#include <kernel/paging.h>
#include <kernel/panic.h>
#include <kernel/spinlock.h>
#include <kernel/vmem.h>

#define BLOCK_FREE 0x80000000
#define BLOCK_END 0x40000000
#define BLOCK_SIZE_MASK 0x7fff

static uint32_t *heap = (uint32_t *)VMEM_HEAP_BASE;
static spinlock_t kmalloc_lock = SPINLOCK_INITIALIZER;

void kmalloc_init(void) {
	heap[0] = (PAGE_SIZE - 4) | BLOCK_FREE | BLOCK_END;
}

void *kmalloc(size_t size) {
	if (size == 0)
		panic("kmalloc: Attempted to allocate 0 bytes");

	size = (size + 3) & ~3;
	if (size + 4 >= 0x1000) {
		return vmalloc((size + 4 + PAGE_SIZE - 1) / PAGE_SIZE, VMA_READ | VMA_WRITE);
	}

	uint64_t flags = spin_acquire_irqsave(&kmalloc_lock);
	uint32_t *block = heap;
	while (true) {
		uint16_t block_size = *block & BLOCK_SIZE_MASK;
		if (*block & BLOCK_FREE) {
			if (block_size >= size) {
				// Found a suitable block
				if (block_size > size + 4) {
					// Split the block
					uint32_t *next_block = block + (size + 4) / 4;
					*next_block = (block_size - size - 4) | ((size + 4) << 15) | BLOCK_FREE;
					*block &= ~BLOCK_END;
				}
				*block = (*block & ~BLOCK_FREE & ~BLOCK_SIZE_MASK) | size;
				spin_release_irqrestore(&kmalloc_lock, flags);
				return block + 1;
			}
		}
		if (*block & BLOCK_END) {
			vmalloc_at((void *)block + 0x1000, (void *)VMEM_HEAP_END, 1, VMA_READ | VMA_WRITE);
			*block += 0x1000;
			continue;
		}
		block = (uint32_t *)((uintptr_t)block + block_size + 4);
	}

	spin_release_irqrestore(&kmalloc_lock, flags);
	return NULL; // No suitable block found
}

void kfree(void *ptr) {
	if (ptr == NULL)
		panic("kfree: Attempted to free a NULL pointer");

	uint64_t flags = spin_acquire_irqsave(&kmalloc_lock);

	uint32_t *block = (uint32_t *)ptr - 1;
	if (*block & BLOCK_FREE)
		panic("kfree: Double free");
	*block |= BLOCK_FREE;

	// Coalesce with next block if it's free
	uint32_t *next_block = (uint32_t *)((uintptr_t)block + (*block & BLOCK_SIZE_MASK) + 4);
	if (*next_block & BLOCK_FREE) {
		uint16_t next_size = *next_block & BLOCK_SIZE_MASK;
		*block = (*block & ~BLOCK_SIZE_MASK) | (next_size + 4);
		if (*next_block & BLOCK_END)
			*block |= BLOCK_END;
	}

	// Coalesce with previous block if it's free
	uint32_t *prev_block = (uint32_t *)((uintptr_t)block - ((*block >> 15) & BLOCK_SIZE_MASK));
	if (block > heap && (*prev_block & BLOCK_FREE)) {
		uint16_t prev_size = *prev_block & BLOCK_SIZE_MASK;
		*prev_block = (*prev_block & ~BLOCK_SIZE_MASK) | (prev_size + (*block & BLOCK_SIZE_MASK) + 4);
		if (*block & BLOCK_END)
			*prev_block |= BLOCK_END;
	}

	spin_release_irqrestore(&kmalloc_lock, flags);
}
