#include <kernel/pmman.h>

#include <stdbool.h>
#include <stdio.h>

#include <kernel/defines.h>
#include <kernel/panic.h>
#include <kernel/tty.h>

#define BUDDY_LAYERS 8
#define BUDDY_MAXLAYER (BUDDY_LAYERS - 1)

static uint8_t *buddy = NULL;
static size_t buddy_off[BUDDY_LAYERS];
static size_t buddy_bitcnts[BUDDY_LAYERS];
static size_t buddy_size;
static uintptr_t max_addr = 0;

static uintptr_t hhdm_off;

static const char *MEM_TYPES[] = {
	"USABLE",
	"RESERVED",
	"ACPI_RECLAIMABLE",
	"ACPI_NVS",
	"BAD_MEMORY",
	"BOOTLOADER_RECLAIMABLE",
	"EXECUTABLE",
	"FRAMEBUFFER",
};

static void buddy_alloc(size_t start, size_t end) {
	for (int k = 0; k < BUDDY_LAYERS; k++) {
		for (size_t i = start; i <= end; i++) {
			size_t byte = i / 8;
			size_t bits = i % 8;
			buddy[buddy_off[k] + byte] &= ~(1 << bits);
		}

		start /= 2;
		end /= 2;
	}
}

static void buddy_free(size_t start, size_t end) {
	// Free the base layer
	for (size_t i = start; i <= end; i++) {
		size_t byte = i / 8;
		size_t bits = i % 8;
		buddy[byte] |= 1 << bits;
	}
	for (int k = 1; k < BUDDY_LAYERS; k++) {
		start /= 2;
		end /= 2;

		for (size_t i = start + 1; i < end; i++) {
			size_t byte = i / 8;
			size_t bits = i % 8;
			buddy[buddy_off[k] + byte] |= 1 << bits;
		}

		// Need to check edge cases (in case it isn't aligned to our block size)
		size_t byte = start / 8;
		size_t bits = start % 8;
		size_t subbyte = start * 2 / 8;
		size_t subbits = (start * 2) % 8;
		uint8_t mask = (1 << subbits) | (1 << (subbits + 1));
		if ((buddy[buddy_off[k - 1] + subbyte] & mask) == mask) {
			buddy[buddy_off[k] + byte] |= (1 << bits);
		} else {
			start++;
		}

		byte = end / 8;
		bits = end % 8;
		subbyte = end * 2 / 8;
		subbits = (end * 2) % 8;
		mask = (1 << subbits) | (1 << (subbits + 1));
		if ((buddy[buddy_off[k - 1] + subbyte] & mask) == mask) {
			buddy[buddy_off[k] + byte] |= (1 << bits);
		} else {
			end--;
		}
		if (end == start - 1 || end == start - 2)
			break;
	}
}

void pmman_init(struct limine_memmap_response *mmap, intptr_t p_hhdm_off, intptr_t kernel_base, intptr_t kernel_size) {
	// Usable and bootloader reclaimable chunks are guaranteed to not overlap and are page aligned
	hhdm_off = p_hhdm_off;
	size_t count = mmap->entry_count;

	size_t mem_size = 0;
	printf("PMMAN: Iterating through %zu memory map entries...\n", count);
	for (size_t i = 0; i < count; i++) {
		struct limine_memmap_entry *entry = mmap->entries[i];
		printf("PMMAN: base=%p size=%p type=%s\n", entry->base, entry->length, MEM_TYPES[entry->type]);
		if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
			max_addr = entry->base + entry->length - 1;
			mem_size += entry->length;
		}
	}
	printf("PMMAN: Maximum usable byte at %p\n", max_addr);
	printf("PMMAN: Usable memory size: %zu bytes (%zu pages)\n", mem_size, mem_size / 0x1000);

	// Calculate sizes and offsets for each layer
	size_t bitcnt = max_addr / 0x1000 + 1;
	for (int i = 1; i < BUDDY_LAYERS; i++) {
		buddy_bitcnts[i - 1] = bitcnt;
		buddy_off[i] = buddy_off[i - 1] + (bitcnt + 7) / BUDDY_LAYERS;
		bitcnt = (bitcnt + 1) / 2;
	}
	buddy_size = buddy_off[BUDDY_MAXLAYER] + (bitcnt + 7) / BUDDY_LAYERS;
	buddy_bitcnts[BUDDY_MAXLAYER] = bitcnt;

	// Put it at the 16MB mark
	/// TODO: Make sure we have enough space here
	buddy = (uint8_t *)(hhdm_off + 0x1000000);
	for (size_t i = 0; i < buddy_size; i++)
		buddy[i] = 0;

	// Mark usable memory
	for (size_t i = 0; i < count; i++) {
		struct limine_memmap_entry *entry = mmap->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
			uintptr_t start = entry->base / 0x1000;
			uintptr_t end = (entry->base + entry->length - 1) / 0x1000;
			buddy_free(start, end);
		}
	}

	// Mark location of buddy allocator as used
	size_t start = (uintptr_t)(buddy - hhdm_off) / 0x1000;
	size_t end = ((uintptr_t)(buddy - hhdm_off) + buddy_size - 1) / 0x1000;
	buddy_alloc(start, end);

	// Make sure NULL page is always invalid (should be guaranteed by memory map but just in case)
	buddy_alloc(0, 0);

	printf("PMMAN: Buddy allocator initialized at %p with size %zu bytes\n", buddy - hhdm_off, buddy_size);
}

static size_t last = 0; // Last page allocated/freed, tries to keep allocations contiguous

struct palloc_info *pmman_alloc(size_t npages, size_t base) {
	if (npages == 0)
		panic("pmman_alloc: Invalid number of pages requested");

	size_t rem = npages; // Remaining pages to allocate
	int layer = BUDDY_MAXLAYER; // Which layer to search in
	struct page_block *head = NULL;
	struct page_block *tail = NULL;
	last = max(last, base); // Start from the last allocated page or the base

	while (rem) {
		while (layer != 0 && (1U << (layer - 1)) >= rem)
			layer--;

	retry:
		// Find our bounds
		size_t start = buddy_off[layer] + (last >> (layer + 3));
		size_t end = buddy_size;
		if (layer != BUDDY_MAXLAYER)
			end = buddy_off[layer + 1];

		bool found = false;
		for (size_t i = start; i < end; i++) {
			if (buddy[i]) {
				// Found a free block
				size_t num = 8 * (i - buddy_off[layer]) + __builtin_ctz(buddy[i]);
				num <<= layer;
				found = true;

				// If it's contiguous with the last allocation, we can extend it
				if (last != base && num == last + 1) {
					size_t nalloc = min(rem, 1U << layer);
					tail->npages += nalloc;
					buddy_alloc(num, num + nalloc - 1);

					rem -= nalloc;
					last += nalloc;
					break;
				}

				// Otherwise, we need to allocate a new block
				struct page_block *block = (struct page_block *)(num * 0x1000 + hhdm_off);
				block->addr = (void *)(num * 0x1000);
				block->npages = min(rem, 1U << layer);
				block->next = NULL;
				buddy_alloc(num, num + block->npages - 1);

				if (head == NULL)
					head = block;
				else
					tail->next = block;
				tail = block;

				rem -= block->npages;
				last = num + block->npages - 1;
				break;
			}
		}

		// Could not find a match from `last` onwards
		if (!found) {
			// First try to search for smaller blocks (helps keep stuff contiguous)
			if (--layer < 0) {
				if (last == base) {
					// Could not find any more free blocks meeting requirements, give up
					break;
				}

				// Could not find anything since `last`, start over from beginning
				last = base;
				layer = BUDDY_MAXLAYER;
				// We want to reshrink the layer
				continue;
			}
			// Retry instead of continue in order to keep the layer the same (we've already searched higher layers)
			goto retry;
		}
	}

	// Used to quickly determine if we gave all requested pages
	if (head == NULL)
		return NULL;
	struct palloc_info *info = (struct palloc_info *)(head + 1);
	info->head = head;
	info->tail = tail;
	info->rem = rem;
	return info;
}

struct palloc_info *kpalloc(size_t npages) {
	if (npages == 1) {
		/// TODO: Use a faster stack allocator for single pages
	}
	// Only allocate from above 16MB
	struct palloc_info *info = pmman_alloc(npages, 0x1000000 / 0x1000);

	// If we have unallocated pages, try to allocate from low memory
	if (info == NULL)
		return pmman_alloc(npages, 0);
	if (info->rem > 0) {
		struct palloc_info *low_info = pmman_alloc(info->rem, 0);
		info->rem = low_info->rem;
		info->tail->next = low_info->head;
		info->tail = low_info->tail;
	}
	return info;
}

/// TODO: Add other more restrictive allocators

void kpfree(void *ptr, size_t npages) {
	size_t start = (uintptr_t)ptr;
	if (start & 0xfff)
		panic("pmman_free: Address must be page-aligned");
	if (start == 0 || npages == 0)
		panic("pmman_free: Invalid parameters");
	start /= 0x1000;

	size_t end = start + npages - 1;
	if (end > max_addr / 0x1000)
		panic("pmman_free: Address out of bounds");

	buddy_free(start, end);

	last = min(last, start);
}
