#include <kernel/vmem.h>

#include <string.h>

#include <x86_64/isr.h>

#include <kernel/mman.h>
#include <kernel/paging.h>

static struct vma *vma_list = NULL;
static struct vma *vma_free_list = NULL;

/// TODO: Fix this when we have a heap manager
static uintptr_t heap_next = VMEM_HEAP_BASE;

static struct vma *alloc_vma(void) {
	if (vma_free_list) {
		struct vma *vma = vma_free_list;
		vma_free_list = vma->next;
		vma->next = NULL;
		return vma;
	}
	// Allocate a new VMA cache block
	map_page((void *)heap_next, (void *)alloc_page(), PAGE_RW | PAGE_NX);
	for (int i = 1; i < PAGE_SIZE / sizeof(struct vma); i++) {
		struct vma *a = (struct vma *)heap_next + (i - 1);
		struct vma *b = (struct vma *)heap_next + i;
		a->next = b;
		b->prev = a;
	}
	vma_free_list = (struct vma *)heap_next;
	heap_next += PAGE_SIZE;
	return (struct vma *)(heap_next - PAGE_SIZE);
}

void free_vma(struct vma *vma) {
	vma->next = vma_free_list;
	vma_free_list = vma;
}

struct isr_frame_t page_fault_handler(struct isr_frame_t frame) {
	uintptr_t addr;
	asm volatile("mov %0, cr2" : "=r"(addr));

	return frame;
}

void vmem_init(void) {
	// Add initial mappings for kernel heap and stack
	create_vma((void *)VMEM_HEAP_BASE, 0, VMA_READ | VMA_WRITE);
	create_vma((void *)VMEM_VIRT_BASE, 0, VMA_READ | VMA_WRITE);
	create_vma((void *)VMEM_MMIO_BASE, 0, VMA_READ | VMA_WRITE);
	create_vma((void *)VMEM_STACK_BASE - KERNEL_STACK_SIZE, KERNEL_STACK_SIZE, VMA_READ | VMA_WRITE);

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
	struct vma *vma = alloc_vma();
	vma->base = base;
	vma->size = size;
	vma->flags = flags;

	struct vma *prev = NULL;
	struct vma *curr = vma_list;
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
