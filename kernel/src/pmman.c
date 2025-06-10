#include <kernel/pmman.h>

#include <kernel/panic.h>

static uint8_t *buddy = NULL;
static size_t buddy_off[8];
static size_t buddy_size;

void init_pmman(struct limine_memmap_response *mmap, intptr_t hhdm_off, intptr_t kernel_base, intptr_t kernel_size) {
	// TODO: Clean up memory map by merging blocks and resolving overlaps

	size_t count = mmap->entry_count;
	size_t max_addr = 0;

	size_t bitcnt = (max_addr + 0xfff) / 0x1000;
	for (int i = 1; i < 8; i++) {
		buddy_off[i] = buddy_off[i - 1] + (bitcnt + 7) / 8;
		buddy_size += buddy_off[i];
		bitcnt = (bitcnt + 1) / 2;
	}

	// TODO: Allocate and populate buddy bitmap

	return 0;
}
