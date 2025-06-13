#include <kernel/pmman.h>

#include <stdio.h>

#include <kernel/panic.h>
#include <kernel/tty.h>

static uint8_t *buddy = NULL;
static size_t buddy_off[8];
static size_t buddy_bitcnts[8];
static size_t buddy_size;
static size_t max_addr = 0;

const char *MEM_TYPES[] = {
	"USABLE",
	"RESERVED",
	"ACPI_RECLAIMABLE",
	"ACPI_NVS",
	"BAD_MEMORY",
	"BOOTLOADER_RECLAIMABLE",
	"EXECUTABLE_AND_MODULES",
	"FRAMEBUFFER",
};

void pmman_init(struct limine_memmap_response *mmap, intptr_t hhdm_off, intptr_t kernel_base, intptr_t kernel_size) {
	// Usable and bootloader reclaimable chunks are guaranteed to not overlap and are page aligned
	size_t count = mmap->entry_count;

	size_t mem_size = 0;
	printf("PMMAN: Iterating through %u memory map entries...\n", count);
	for (size_t i = 0; i < count; i++) {
		struct limine_memmap_entry *entry = mmap->entries[i];
		printf("PMMAN: base=0x%x length=0x%x type=%s\n", entry->base, entry->length, MEM_TYPES[entry->type]);
		if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
			max_addr = entry->base + entry->length - 1;
			mem_size += entry->length;
		}
	}
	printf("PMMAN: Maximum usable byte at 0x%x\n", max_addr);
	printf("PMMAN: Usable memory size: %u bytes\n", mem_size);

	size_t bitcnt = max_addr / 0x1000 + 1;
	for (int i = 1; i < 8; i++) {
		buddy_bitcnts[i - 1] = bitcnt;
		buddy_off[i] = buddy_off[i - 1] + (bitcnt + 7) / 8;
		bitcnt = (bitcnt + 1) / 2;
	}
	buddy_size = buddy_off[7] + (bitcnt + 7) / 8;
	buddy_bitcnts[7] = bitcnt;

	// Put it at the 16MB mark
	buddy = (uint8_t *)(hhdm_off + 0x1000000);
	for (size_t i = 0; i < buddy_size; i++)
		buddy[i] = 0;

	// Populate buddy allocator
	for (size_t i = 0; i < count; i++) {
		struct limine_memmap_entry *entry = mmap->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
			uintptr_t start = entry->base / 0x1000;
			uintptr_t end = (entry->base + entry->length - 1) / 0x1000;
			for (size_t j = start; j <= end; j++) {
				size_t byte = j / 8;
				size_t bits = j % 8;
				buddy[byte] |= (1 << bits);
			}
		}
	}
	size_t start = (uintptr_t)(buddy - hhdm_off) / 0x1000;
	size_t end = ((uintptr_t)(buddy - hhdm_off) + buddy_size - 1) / 0x1000;
	for (size_t j = start; j <= end; j++) {
		size_t byte = j / 8;
		size_t bits = j % 8;
		buddy[byte] ^= (1 << bits);
	}
	for (int i = 1; i < 8; i++) {
		for (size_t j = 0; j < buddy_bitcnts[i]; j++) {
			size_t byte = j / 8;
			size_t bits = j % 8;
			size_t subbytes = j * 2 / 8;
			size_t subbits = (j * 2) % 8;
			uint8_t mask = (1 << bits) | (1 << (bits + 1));
			if ((buddy[buddy_off[i - 1] + subbytes] & mask) == mask)
				buddy[buddy_off[i] + byte] |= (1 << bits);
		}
	}
	printf("PMMAN: Buddy allocator initialized at 0x%x with size %u bytes\n", buddy, buddy_size);
}
