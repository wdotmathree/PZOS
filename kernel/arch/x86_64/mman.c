#include <kernel/mman.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <kernel/defines.h>
#include <kernel/log.h>
#include <kernel/paging.h>
#include <kernel/panic.h>
#include <kernel/tty.h>
#include <kernel/vmem.h>

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

static const size_t bitmap_size = 0x1000000 / 0x1000 / 8;
static uint64_t *bitmap;
static uint64_t *page_stack;
static uint64_t *page_stack_base;

uintptr_t hhdm_off;

// Our definition of "high memory" is everything above **16MiB** (not usable for ISA DMA) instead of the usual 1MiB
void mman_init(struct limine_memmap_response *mmap, uint8_t **framebuf, uintptr_t hhdm_offset, uintptr_t kernel_end) {
	// Usable and bootloader reclaimable chunks are guaranteed to not overlap and are page aligned
	size_t count = mmap->entry_count;
	size_t max_addr = 0;

	uintptr_t framebuf_base = 0;
	intptr_t framebuf_size = 0;

	hhdm_off = hhdm_offset;

	// Print memory map entries for logging
	size_t mem_size = 0;
	LOG("MMAN", "Iterating through %zu memory map entries...", count);
	for (size_t i = 0; i < count; i++) {
		struct limine_memmap_entry *entry = mmap->entries[i];
		LOG("MMAN", "base=%p size=%p type=%s", entry->base, entry->length, MEM_TYPES[entry->type]);
		if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
			max_addr = entry->base + entry->length - 1;
			mem_size += entry->length;
		} else if (entry->type == LIMINE_MEMMAP_FRAMEBUFFER) {
			// We only use the first framebuffer
			if (framebuf_base == 0) {
				framebuf_base = entry->base;
				framebuf_size = entry->length;
			}
		}
	}
	LOG("MMAN", "Maximum usable byte at %p", max_addr);
	LOG("MMAN", "Usable memory size: %zu bytes (%zu pages)", mem_size, mem_size / 0x1000);

	// Find candidates for low memory bitmap and high memory page stack
	size_t reserve = ((max_addr - 0x1000000) / 0x1000 * 8 + bitmap_size + 0xfff) / 0x1000; // How many pages we reserve for the memory management stuff
	for (size_t i = 0; i < count; i++) {
		struct limine_memmap_entry *entry = mmap->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			if (entry->base + entry->length > max(entry->base, 0x1000000) + reserve * 0x1000) {
				LOG("MMAN", "Reserving %zu pages for memory management structures at %p", reserve, entry->base);
				bitmap = (uint64_t *)(max(entry->base, 0x1000000) + hhdm_off);
				break;
			}
		}
	}
	if (bitmap == NULL) {
		// Should never happen, but just in case
		panic("MMAN: Not enough usable memory for bitmap and page stack");
	}

	// Setup a bitmap for low memory (placed at beginning of high memory)
	// Don't include bootloader reclaimable memory yet
	memset(bitmap, 0, bitmap_size);
	for (size_t i = 0; i < count; i++) {
		struct limine_memmap_entry *entry = mmap->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			size_t start_page = entry->base / 0x1000;
			size_t end_page = (entry->base + entry->length - 1) / 0x1000;
			uint64_t mask = 1ULL << (start_page % 64);
			for (size_t j = start_page; j <= min(end_page, 0x1000000 / 0x1000 - 1); j++) {
				bitmap[j / 64] |= mask;
				asm("rol %0, 1" : "+r"(mask));
			}
		}
	}

	// Setup a page stack for high memory (placed right after bitmap)
	page_stack_base = page_stack = (uint64_t *)((uintptr_t)bitmap + bitmap_size);
	page_stack--;
	for (int i = count - 1; i >= 0; i--) {
		struct limine_memmap_entry *entry = mmap->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			size_t start_page = entry->base / 0x1000;
			size_t end_page = (entry->base + entry->length - 1) / 0x1000;
			for (size_t j = end_page; j >= max(start_page, 0x1000000 / 0x1000); j--) {
				// Skip the pages we reserved for memory management
				if (j == ((uintptr_t)bitmap - hhdm_off) / 0x1000 + reserve - 1) {
					j -= reserve;
					if (j < max(start_page, 0x1000000 / 0x1000))
						continue;
				}
				*++page_stack = j;
			}
		}
	}

	// Now that we can allocate pages, make the page tables
	// We have to allocate and map manually for now until we switch to our new page tables
	uint64_t *pml4 = (uint64_t *)(hhdm_off + alloc_page());
	memset(pml4, 0, 0x1000);

	// Map kernel stack (page 0xffff7ffff)
	uint64_t *pdpt = (uint64_t *)(hhdm_off + alloc_page());
	pml4[0x1ff] = ((uintptr_t)pdpt - hhdm_off) | PAGE_PRESENT | PAGE_RW;
	memset(pdpt, 0, 0x1000);
	uint64_t *pd = (uint64_t *)(hhdm_off + alloc_page());
	pdpt[0x1fd] = ((uintptr_t)pd - hhdm_off) | PAGE_PRESENT | PAGE_RW | PAGE_NX;
	memset(pd, 0, 0x1000);
	uint64_t *pt = (uint64_t *)(hhdm_off + alloc_page());
	pd[0x1ff] = ((uintptr_t)pt - hhdm_off) | PAGE_PRESENT | PAGE_RW | PAGE_NX;
	memset(pt, 0, 0x1000);
	uintptr_t stack_base = 0;
	asm("mov %0, [rbp]" : "=r"(stack_base));
	pt[0x1ff] = (stack_base - 0x1000 - hhdm_off) | PAGE_PRESENT | PAGE_RW | PAGE_NX | PAGE_TYPE(PAT_WB);

	// Map framebuffer
	uintptr_t ptr = framebuf_base;
	uintptr_t vptr = 0xfffff80000000000;
	pdpt = (uint64_t *)(hhdm_off + alloc_page());
	pml4[0x1f0] = ((uintptr_t)pdpt - hhdm_off) | PAGE_PRESENT | PAGE_RW | PAGE_NX;
	memset(pdpt, 0, 0x1000);
	pd = (uint64_t *)(hhdm_off + alloc_page());
	pdpt[0x000] = ((uintptr_t)pd - hhdm_off) | PAGE_PRESENT | PAGE_RW | PAGE_NX;
	memset(pd, 0, 0x1000);
	if ((ptr & 0x1fffff) == 0) {
		// Framebuffer is large page aligned
		while (framebuf_size >= 0x200000) {
			pd[LINADDR_PDE(vptr)] = ptr | PAGE_PRESENT | PAGE_RW | PAGE_NX | PAGE_PS | PAGE_TYPE(PAT_WC);
			ptr += 0x200000;
			vptr += 0x200000;
			framebuf_size -= 0x200000;
		}
	}
	while (framebuf_size > 0) {
		if (pd[LINADDR_PDE(vptr)] == 0) {
			pt = (uint64_t *)(hhdm_off + alloc_page());
			pd[LINADDR_PDE(vptr)] = ((uintptr_t)pt - hhdm_off) | PAGE_PRESENT | PAGE_RW | PAGE_NX;
			memset(pt, 0, 0x1000);
		}
		pt[LINADDR_PTE(vptr)] = ptr | PAGE_PRESENT | PAGE_RW | PAGE_NX | PAGE_TYPE(PAT_WC);
		ptr += 0x1000;
		vptr += 0x1000;
		framebuf_size -= 0x1000;
	}

	// Recursive page table entry
	pml4[0x1fe] = ((uintptr_t)pml4 - hhdm_off) | PAGE_PRESENT | PAGE_RW | PAGE_NX | PAGE_PWT;

	// Find old page tables for kernel mappings
	uint64_t *old_pml4;
	asm("mov %0, cr3" : "=r"(old_pml4));
	old_pml4 = (uint64_t *)(hhdm_off + (uintptr_t)old_pml4);

	// Map kernel executable
	// Go through old page tables and copy
	pdpt = (uint64_t *)(hhdm_off + TABLE_ENTRY_ADDR(pml4[0x1ff]));
	pd = (uint64_t *)(hhdm_off + alloc_page());
	pdpt[0x1fe] = ((uintptr_t)pd - hhdm_off) | PAGE_PRESENT | PAGE_RW;
	memset(pd, 0, 0x1000);
	uint64_t *old_pdpt = (uint64_t *)(hhdm_off + TABLE_ENTRY_ADDR(old_pml4[0x1ff]));
	uint64_t *old_pd = (uint64_t *)(hhdm_off + TABLE_ENTRY_ADDR(old_pdpt[0x1fe]));
	for (size_t j = 0; j < 512; j++) {
		if (old_pd[j] & PAGE_PRESENT) {
			pt = (uint64_t *)(hhdm_off + alloc_page());
			pd[j] = ((uintptr_t)pt - hhdm_off) | (old_pd[j] & (PAGE_PRESENT | PAGE_RW | PAGE_PWT | PAGE_PCD | PAGE_PS | PAGE_NX));
			memset(pt, 0, 0x1000);
			uint64_t *old_pt = (uint64_t *)(hhdm_off + TABLE_ENTRY_ADDR(old_pd[j]));
			for (size_t k = 0; k < 512; k++) {
				pt[k] = old_pt[k] & ~(PAGE_USER | PAGE_ACCESSED | PAGE_DIRTY);
			}
		}
	}

	// Direct map
	ptr = 0;
	while (max_addr - ptr >= 0x40000000) {
		if (pml4[0x100 + LINADDR_PML4E(ptr)] == 0) {
			pdpt = (uint64_t *)(hhdm_off + alloc_page());
			pml4[0x100 + LINADDR_PML4E(ptr)] = ((uintptr_t)pdpt - hhdm_off) | PAGE_PRESENT | PAGE_RW | PAGE_NX;
			memset(pdpt, 0, 0x1000);
		}
		pdpt[LINADDR_PDPTE(ptr)] = ptr | PAGE_PRESENT | PAGE_RW | PAGE_NX | PAGE_PS | PAGE_TYPE(PAT_WC);
		ptr += 0x40000000;
	}
	while (max_addr - ptr >= 0x200000) {
		if (pml4[0x100 + LINADDR_PML4E(ptr)] == 0) {
			pdpt = (uint64_t *)(hhdm_off + alloc_page());
			pml4[0x100 + LINADDR_PML4E(ptr)] = ((uintptr_t)pdpt - hhdm_off) | PAGE_PRESENT | PAGE_RW | PAGE_NX;
			memset(pdpt, 0, 0x1000);
		}
		if (pdpt[LINADDR_PDPTE(ptr)] == 0) {
			pd = (uint64_t *)(hhdm_off + alloc_page());
			pdpt[LINADDR_PDPTE(ptr)] = ((uintptr_t)pd - hhdm_off) | PAGE_PRESENT | PAGE_RW | PAGE_NX;
			memset(pd, 0, 0x1000);
		}
		pd[LINADDR_PDE(ptr)] = ptr | PAGE_PRESENT | PAGE_RW | PAGE_NX | PAGE_PS | PAGE_TYPE(PAT_WC);
		ptr += 0x200000;
	}
	while (max_addr - ptr < 0x200000) { // Detect overflow
		if (pml4[0x100 + LINADDR_PML4E(ptr)] == 0) {
			pdpt = (uint64_t *)(hhdm_off + alloc_page());
			pml4[0x100 + LINADDR_PML4E(ptr)] = ((uintptr_t)pdpt - hhdm_off) | PAGE_PRESENT | PAGE_RW | PAGE_NX;
			memset(pdpt, 0, 0x1000);
		}
		if (pdpt[LINADDR_PDPTE(ptr)] == 0) {
			pd = (uint64_t *)(hhdm_off + alloc_page());
			pdpt[LINADDR_PDPTE(ptr)] = ((uintptr_t)pd - hhdm_off) | PAGE_PRESENT | PAGE_RW | PAGE_NX;
			memset(pd, 0, 0x1000);
		}
		if (pd[LINADDR_PDE(ptr)] == 0) {
			pt = (uint64_t *)(hhdm_off + alloc_page());
			pd[LINADDR_PDE(ptr)] = ((uintptr_t)pt - hhdm_off) | PAGE_PRESENT | PAGE_RW | PAGE_NX;
			memset(pt, 0, 0x1000);
		}
		pt[LINADDR_PTE(ptr)] = ptr | PAGE_PRESENT | PAGE_RW | PAGE_NX | PAGE_TYPE(PAT_WC);
		ptr += 0x1000;
	}

	// Switch to our new page tables
	asm("add rsp, %0\n\t"
		"add rbp, %0\n\t"
		"mov cr3, %1" : : "g"(0xffffffff80000000 - stack_base),
						  "r"((uintptr_t)pml4 - hhdm_off), "n"(KERNEL_CS));

	LOG("MMAN", "Page tables initialized successfully.");
	LOG("MMAN", "PML4 is at %p", (uintptr_t)pml4 - hhdm_off);

	// Reclaim bootloader reclaimable memory
	for (size_t i = 0; i < count; i++) {
		struct limine_memmap_entry *entry = mmap->entries[i];
		if (entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
			size_t start_page = entry->base / 0x1000;
			size_t end_page = (entry->base + entry->length - 1) / 0x1000;
			for (size_t j = start_page; j <= end_page; j++) {
				// Make sure we don't reclaim the page for the top of stack
				if (j == stack_base / 0x1000)
					continue;
				if (j < 0x1000000 / 0x1000)
					bitmap[j / 64] |= (1ULL << (j % 64));
				else
					*++page_stack = j;
			}
		}
	}

	LOG("MMAN", "Memory management initialized successfully.");
}

void *alloc_page(void) {
	if (page_stack < page_stack_base) {
		// If stack is empty, try to allocate from low memory
		for (size_t i = 0; i < bitmap_size; i++) {
			if (bitmap[i]) {
				int bit = __builtin_ctzll(bitmap[i]);
				bitmap[i] &= ~(1ULL << bit);
				return (void *)((i * 64 + bit) * 0x1000);
			}
		}
		panic("alloc_page: No free pages available");
	}
	// Allocate from the stack
	return (void *)(*page_stack-- * 0x1000);
}

void free_page(void *ptr) {
	size_t page_num = (uintptr_t)ptr / 0x1000;
	if ((uintptr_t)ptr < 0x1000000) {
		// Free from bitmap
		bitmap[page_num / 64] |= (1ULL << (page_num % 64));
	} else {
		*++page_stack = page_num;
	}
}

void *alloc_contig(size_t npages) {
	if (npages == 0)
		return NULL;

	// We always allocate from low memory so we can use the bitmap
	size_t cnt = 0;
	uint64_t mask = 1;
	for (size_t i = 0; i < bitmap_size * 8; i++) {
		if (bitmap[i / 64] & mask) {
			if (++cnt == npages) {
				// We found a contiguous block of pages
				for (size_t j = 0; j < npages; j++) {
					bitmap[(i - cnt + j) / 64] &= ~(1ULL << ((i - cnt + j) % 64));
				}
				return (void *)((i - cnt + 1) * 0x1000);
			}
		} else {
			cnt = 0;
		}
		asm("rol %0, 1" : "+r"(mask));
	}
	panic("alloc_contig: Not enough contiguous pages available");
}

void free_contig(void *ptr, size_t npages) {
	if (npages == 0 || ptr == NULL)
		return;

	size_t page_num = (uintptr_t)ptr / 0x1000;
	for (size_t i = 0; i < npages; i++) {
		bitmap[page_num / 64] |= (1ULL << (page_num % 64));
	}
}
