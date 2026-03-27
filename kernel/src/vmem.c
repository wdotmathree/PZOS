#include <kernel/vmem.h>

#include <stdio.h>
#include <string.h>

#include <x86_64/irq.h>

#include <kernel/mman.h>
#include <kernel/paging.h>
#include <kernel/panic.h>
#include <kernel/spinlock.h>

vma_list_t *vma_list = NULL;
vma_list_t *vma_free_list = NULL;

spinlock_t vma_lock = SPINLOCK_INITIALIZER;

static vma_list_t *alloc_vma(void) {
	if (vma_free_list) {
		vma_list_t *vma = vma_free_list;
		vma_free_list = vma->next;
		return vma;
	}

	// Find a free virtual page
	vma_list_t *free_addr = (vma_list_t *)vmalloc(PAGE_SIZE, VMA_READ | VMA_WRITE);
	// Allocate a new VMA cache block
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
	vma_free_list = __va(alloc_page());
	for (size_t i = 1; i < PAGE_SIZE / sizeof(vma_list_t); i++) {
		vma_list_t *a = &vma_free_list[i - 1];
		vma_list_t *b = &vma_free_list[i];
		a->next = b;
		b->prev = a;
	}

	// Finish setting up the BSP stack
	uintptr_t kernel_stack_bottom = VMEM_STACK_BASE - KERNEL_STACK_SIZE;
	for (size_t i = 0; i < KERNEL_STACK_SIZE - PAGE_SIZE; i += PAGE_SIZE) {
		void *page = (void *)(kernel_stack_bottom + i);
		early_map(NULL, page, alloc_page(), MAP_SIZE_4K, PAGE_RW | PAGE_NX);
	}

	// Set up the pageinfo VMA
	create_vma((void *)VMEM_PAGEINFO_BASE, VMEM_PAGEINFO_MAXSIZE, VMA_READ | VMA_WRITE | VMA_ZERO);

	// Register the page fault handler
	register_isr(14, page_fault_handler, 0);
}

void create_vma(void *base, size_t size, uint64_t flags) {
	uint64_t prev_irq = spin_acquire_irqsave(&vma_lock);

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

	spin_release_irqrestore(&vma_lock, prev_irq);
}

void destroy_vma(vma_list_t *vma) {
	uint64_t flags = spin_acquire_irqsave(&vma_lock);

	if (vma->prev)
		vma->prev->next = vma->next;
	else
		vma_list = vma->next;

	if (vma->next)
		vma->next->prev = vma->prev;

	free_vma(vma);

	spin_release_irqrestore(&vma_lock, flags);
}

void *vmalloc_at(void *start, void *end, size_t npages, uint64_t flags) {
	uint64_t prev_irq = spin_acquire_irqsave(&vma_lock);

	if (vma_list == NULL) {
		// First VMA
		void *addr = start;
		vma_list = alloc_vma();
		vma_list->base = addr;
		vma_list->size = npages * PAGE_SIZE;
		vma_list->flags = flags;
		vma_list->next = NULL;
		vma_list->prev = NULL;
		spin_release_irqrestore(&vma_lock, prev_irq);
		return addr;
	}
	if (vma_list->next == NULL) {
		// Only one VMA
		if (start + npages * PAGE_SIZE <= vma_list->base) {
			// Allocate before
			void *addr = start;
			vma_list_t *new_vma = alloc_vma();
			new_vma->base = addr;
			new_vma->size = npages * PAGE_SIZE;
			new_vma->flags = flags;
			new_vma->next = vma_list;
			new_vma->prev = NULL;
			vma_list->prev = new_vma;
			vma_list = new_vma;
			spin_release_irqrestore(&vma_lock, prev_irq);
			return addr;
		} else if (vma_list->base + vma_list->size + npages * PAGE_SIZE <= end) {
			// Allocate after
			void *addr = vma_list->base + vma_list->size;
			vma_list_t *new_vma = alloc_vma();
			new_vma->base = addr;
			new_vma->size = npages * PAGE_SIZE;
			new_vma->flags = flags;
			vma_list->next = new_vma;
			new_vma->prev = vma_list;
			new_vma->next = NULL;
			spin_release_irqrestore(&vma_lock, prev_irq);
			return addr;
		} else {
			// No space
			panic("Virtual address space exhausted, cannot allocate VMA");
		}
	}

	vma_list_t *prev = vma_list;
	vma_list_t *curr = vma_list->next;
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

				spin_release_irqrestore(&vma_lock, prev_irq);
				return free_addr;
			}
			if (prev->base >= end)
				break;
		}
		prev = curr;
		curr = curr->next;
	}
	// Allocate at the end if possible
	if (prev->base + prev->size + npages * PAGE_SIZE <= end) {
		void *addr = prev->base + prev->size;
		vma_list_t *new_vma = alloc_vma();
		new_vma->base = addr;
		new_vma->size = npages * PAGE_SIZE;
		new_vma->flags = flags;
		prev->next = new_vma;
		new_vma->prev = prev;
		new_vma->next = NULL;
		spin_release_irqrestore(&vma_lock, prev_irq);
		return addr;
	}

	// Ran out of space (somehow)
	panic("Virtual address space exhausted, cannot allocate VMA");
}

void *vmalloc(size_t npages, uint64_t flags) {
	return vmalloc_at((void *)VMEM_VIRT_BASE, (void *)VMEM_VIRT_END, npages, flags);
}

void vfree(void *addr, size_t npages) {
	// Go to beginning of page
	addr = (void *)((uintptr_t)addr & ~(PAGE_SIZE - 1));

	uint64_t flags = spin_acquire_irqsave(&vma_lock);

	// Find the VMA that contains the address
	vma_list_t *prev = vma_list;
	vma_list_t *curr = vma_list->next;
	while (curr) {
		if (curr->base > addr)
			break;
		prev = curr;
		curr = curr->next;
	}

	// Check if we actually found one or just didn't match anything
	if (prev->base <= addr) {
		if (addr + npages * PAGE_SIZE > prev->base + prev->size) // We cannot overflow into another VMA
			panic("vfree: Invalid free size");
		if (addr == prev->base && npages * PAGE_SIZE == prev->size) {
			// Delete the whole VMA
			destroy_vma(prev);
		} else if (addr == prev->base) {
			// Shrink from the start
			prev->base = prev->base + npages * PAGE_SIZE;
		} else if (addr + npages * PAGE_SIZE == prev->base + prev->size) {
			// Shrink from the end
			prev->size -= npages * PAGE_SIZE;
		} else {
			// Split into two VMAs
			prev->size = addr - prev->base;
			vma_list_t *new_vma = alloc_vma();
			new_vma->base = addr + npages * PAGE_SIZE;
			new_vma->size = (prev->base + prev->size) - new_vma->base;
			new_vma->next = prev->next;
			new_vma->prev = prev;
			prev->next = new_vma;
		}
	} else {
		panic("vfree: Invalid free address");
	}

	spin_release_irqrestore(&vma_lock, flags);
}
