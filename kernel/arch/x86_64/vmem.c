#include <kernel/vmem.h>

#include <stdio.h>
#include <string.h>

#include <x86_64/isr.h>

#include <kernel/mman.h>
#include <kernel/paging.h>
#include <kernel/panic.h>

static vma_list_t *vma_list = NULL;
static vma_list_t *vma_free_list = NULL;

isr_frame_t *page_fault_handler(isr_frame_t *const frame) {
	uintptr_t addr;
	uint16_t error = frame->error_code;
	asm volatile("mov %0, cr2" : "=r"(addr));

	// Check if the fault was caused by a user-mode access
	if (error & PF_USER) {
		/// TODO: Handle user-mode page fault
		panic("User-mode page fault at address %p", (void *)addr);
	}

	// Find a VMA that covers the faulting address
	vma_list_t *vma = vma_list;
	while (vma) {
		if ((uintptr_t)vma->base <= addr && addr < (uintptr_t)vma->base + vma->size)
			break;
		vma = vma->next;
	}
	if (vma == NULL)
		panic("Page fault at address %p, no VMA found", (void *)addr);

	// Right now we only handle demand paging, so we will allocate a new page
	/// TODO: Handle other cases (CoW, file mappings, etc.)
	/// TODO: Handle flags properly
	map_page((void *)addr, alloc_page(), PAGE_PRESENT | PAGE_RW | PAGE_NX);

	return frame;
}

static vma_list_t *alloc_vma(void) {
	if (vma_free_list) {
		vma_list_t *vma = vma_free_list;
		vma_free_list = vma->next;
		return vma;
	}

	// Find a free virtual page
	vma_list_t *free_addr = (vma_list_t *)vmalloc(PAGE_SIZE, VMA_READ | VMA_WRITE);
	// Allocate a new VMA cache block
	// map_page(free_addr, (void *)alloc_page(), PAGE_RW | PAGE_NX);
	for (size_t i = 1; i < PAGE_SIZE / sizeof(vma_list_t); i++) {
		vma_list_t *a = &free_addr[i - 1];
		vma_list_t *b = &free_addr[i];
		a->next = b;
		b->prev = a;
	}

	// Otherwise we return the first new VMA
	vma_free_list = &free_addr[1];
	return free_addr;
}

static void free_vma(vma_list_t *vma) {
	vma->next = vma_free_list;
	vma_free_list = vma;
}

void vmem_init(void) {
	// Initialize the VMA list
	map_page((void *)VMEM_VIRT_BASE, (void *)alloc_page(), PAGE_RW | PAGE_NX);
	vma_free_list = (vma_list_t *)VMEM_VIRT_BASE;
	for (size_t i = 1; i < PAGE_SIZE / sizeof(vma_list_t); i++) {
		vma_list_t *a = &vma_free_list[i - 1];
		vma_list_t *b = &vma_free_list[i];
		a->next = b;
		b->prev = a;
	}

	// Add initial mappings for kernel heap and stack
	create_vma((void *)VMEM_STACK_BASE - KERNEL_STACK_SIZE, KERNEL_STACK_SIZE, VMA_READ | VMA_WRITE);
	create_vma((void *)VMEM_MMIO_BASE, 0, VMA_READ | VMA_WRITE);
	create_vma((void *)VMEM_VIRT_BASE, PAGE_SIZE, VMA_READ | VMA_WRITE); // One page for the preallocated VMA's
	create_vma((void *)VMEM_HEAP_BASE, PAGE_SIZE, VMA_READ | VMA_WRITE); // We need at least one page for heap, so might as well allocate it now

	// Finish setting up the kernel stack
	uintptr_t kernel_stack_bottom = VMEM_STACK_BASE - KERNEL_STACK_SIZE;
	for (size_t i = 0; i < KERNEL_STACK_SIZE - PAGE_SIZE; i += PAGE_SIZE) {
		void *page = (void *)((uintptr_t)kernel_stack_bottom + i);
		map_page(page, alloc_page(), PAGE_RW | PAGE_NX);
		invlpg(page);
	}

	// Register the page fault handler
	register_isr(14, page_fault_handler, 0);
}

void create_vma(void *base, size_t size, uint64_t flags) {
	vma_list_t *vma = alloc_vma();
	vma->base = base;
	vma->size = size;
	vma->flags = flags;

	vma_list_t *prev = NULL;
	vma_list_t *curr = vma_list;
	while (curr && curr->base < base) {
		prev = curr;
		curr = curr->next;
	}
	if (prev == NULL) {
		// Insert at the beginning
		vma->next = vma_list;
		if (vma_list)
			vma_list->prev = vma;
		vma_list = vma;
	} else {
		// Insert in the middle or end
		prev->next = vma;
		vma->prev = prev;
		vma->next = curr;
		if (curr)
			curr->prev = vma;
	}
}

void destroy_vma(vma_list_t *vma) {
	if (vma->prev)
		vma->prev->next = vma->next;
	else
		vma_list = vma->next;

	if (vma->next)
		vma->next->prev = vma->prev;

	free_vma(vma);
}

void *vmalloc_at(void *start, void *end, size_t npages, uint64_t flags) {
	vma_list_t *prev = vma_list;
	vma_list_t *curr = vma_list->next; // Guaranteed to exist since we always have at least 4 mappings
	while (curr) {
		if (prev->base + prev->size >= start) {
			if (prev->base + prev->size + npages * PAGE_SIZE <= curr->base) {
				// Found a gap
				void *free_addr = (void *)(prev->base + prev->size);
				if (prev->flags == flags) {
					// Expand left VMA
					prev->size += npages * PAGE_SIZE;
					if (prev->base + prev->size == curr->base && curr->flags == flags) {
						// Combine with right VMA
						prev->size += curr->size;
						prev->next = curr->next;
						if (curr->next)
							curr->next->prev = prev;
						free_vma(curr);
					}
				} else if (free_addr + npages * PAGE_SIZE == curr->base && curr->flags == flags) {
					// Expand right VMA
					curr->base = free_addr;
					curr->size += npages * PAGE_SIZE;
				} else {
					// Create a new VMA in the gap
					create_vma(free_addr, npages * PAGE_SIZE, flags);
				}
				return free_addr;
			}
			if (prev->base >= end)
				break;
		}
		prev = curr;
		curr = curr->next;
	}
	// Ran out of space (somehow)
	panic("Virtual address space exhausted, cannot allocate VMA");
}

void *vmalloc(size_t npages, uint64_t flags) {
	return vmalloc_at((void *)VMEM_VIRT_BASE, (void *)VMEM_VIRT_END, npages, flags);
}
