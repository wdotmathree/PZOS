#include <kernel/paging.h>

#include <stdio.h>
#include <string.h>

#include <kernel/log.h>
#include <kernel/mman.h>
#include <kernel/pageinfo.h>
#include <kernel/panic.h>
#include <kernel/spinlock.h>
#include <kernel/vmem.h>

extern vma_list_t *vma_list;

extern spinlock_t vma_lock;

isr_frame_t *page_fault_handler(isr_frame_t *const frame) {
	void *addr;
	uint16_t error = frame->error_code;
	asm volatile("mov %0, cr2" : "=r"(addr));

	// Check if the fault was caused by a user-mode access
	if (error & PF_USER) {
		/// TODO: Handle user-mode page fault
		panic("User-mode page fault at address %p", (void *)addr);
	}

	// Find a VMA that covers the faulting address
	spin_acquire(&vma_lock);
	vma_list_t *vma = vma_list;
	while (vma) {
		if (vma->base <= addr && addr < vma->base + vma->size)
			break;
		vma = vma->next;
	}
	spin_release(&vma_lock);
	if (vma == NULL)
		panic("Page fault at address %p RIP %p no VMA found", (void *)addr, frame->isr_rip);

	// Right now we only handle demand paging, so we will allocate a new page
	if (vma->base == VMEM_PAGEINFO_BASE) {
		early_map(NULL, addr, alloc_page(), MAP_SIZE_4K, PAGE_PRESENT | PAGE_RW | PAGE_NX);
	} else {
		map_page(addr, alloc_page(), PAGE_PRESENT | PAGE_RW | PAGE_NX);
	}

	if (vma->flags & VMA_ZERO) {
		// Zero the newly allocated page
		void *page_base = (void *)((uintptr_t)addr & ~(PAGE_SIZE - 1));
		memset(page_base, 0, PAGE_SIZE);
	}

	return frame;
}

static spinlock_t paging_lock = SPINLOCK_INITIALIZER;

static size_t cnt = 0;
// Ignores refcounting and locking, assumes single-threaded and no duplicate mappings
void early_map(uint64_t *pml4, const void *virt_addr, const physaddr_t phys_addr, enum map_size size, uint64_t flags) {
	if (pml4 == NULL) {
		asm("mov %0, cr3" : "=r"(pml4));
		pml4 = __va(pml4);
	}

	physaddr_t phys = phys_addr & -0x1000LL;
	uintptr_t virt = (uintptr_t)virt_addr & -0x1000LL;
	if (virt_addr == NULL)
		panic("early_map: No virtual address provided");

	if (size != MAP_SIZE_4K)
		flags |= PAGE_PS;

	uint64_t *pml4e = pml4 + LINADDR_PML4I(virt);
	uint64_t *pdpte;
	uint64_t *pde;
	uint64_t *pte;
	cnt++;
	if ((*pml4e & PAGE_PRESENT) == 0) {
		physaddr_t addr = alloc_page();
		*pml4e = addr | PAGE_PRESENT | PAGE_RW;
		memset((void *)__va(addr), 0, 0x1000);
	}
	pdpte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pml4e)) + LINADDR_PDPTI(virt);

	if (size == MAP_SIZE_1G) {
		*pdpte = phys | PAGE_PRESENT | PAGE_RW | flags;
		invlpg(virt);
		return;
	}
	if ((*pdpte & PAGE_PRESENT) == 0) {
		if (*pdpte)
			LOG(__FUNCTION__, "Warning: Overwriting existing PDE entry %p", (void *)virt);
		physaddr_t addr = alloc_page();
		*pdpte = addr | PAGE_PRESENT | PAGE_RW;
		memset((void *)__va(addr), 0, 0x1000);
	}
	pde = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pdpte)) + LINADDR_PDI(virt);

	if (size == MAP_SIZE_2M) {
		if (*pde)
			LOG(__FUNCTION__, "Warning: Overwriting existing PDE entry %p", (void *)virt);
		*pde = phys | PAGE_PRESENT | PAGE_RW | flags;
		invlpg(virt);
		return;
	}
	if ((*pde & PAGE_PRESENT) == 0) {
		physaddr_t addr = alloc_page();
		*pde = addr | PAGE_PRESENT | PAGE_RW;
		memset((void *)__va(addr), 0, 0x1000);
	}
	pte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pde)) + LINADDR_PTI(virt);

	*pte = phys | PAGE_PRESENT | flags;
	invlpg(virt);
}

void map_page(const void *virt_addr, const physaddr_t phys_addr, uint64_t flags) {
	physaddr_t phys = phys_addr & -0x1000LL;
	uintptr_t virt = (uintptr_t)virt_addr & -0x1000LL;
	if (virt_addr == NULL)
		panic("map_page: No virtual address provided");

	uint64_t *pml4e = (uint64_t *)LINADDR_PML4E_PTR(virt);
	uint64_t *pdpte;
	uint64_t *pde;
	uint64_t *pte;

	uint64_t prev_irq = spin_acquire_irqsave(&paging_lock);

	if ((*pml4e & PAGE_PRESENT) == 0) {
		physaddr_t addr = alloc_page();
		*pml4e = addr | PAGE_PRESENT | PAGE_RW;
		memset((void *)__va(addr), 0, 0x1000);
	}
	pdpte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pml4e)) + LINADDR_PDPTI(virt);

	if ((*pdpte & PAGE_PRESENT) == 0) {
		physaddr_t addr = alloc_page();
		*pdpte = addr | PAGE_PRESENT | PAGE_RW;
		memset((void *)__va(addr), 0, 0x1000);
		inc_page(TABLE_ENTRY_ADDR(*pml4e));
	}
	pde = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pdpte)) + LINADDR_PDI(virt);

	if ((*pde & PAGE_PRESENT) == 0) {
		physaddr_t addr = alloc_page();
		*pde = addr | PAGE_PRESENT | PAGE_RW;
		memset((void *)__va(addr), 0, 0x1000);
		inc_page(TABLE_ENTRY_ADDR(*pdpte));
	}
	pte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pde)) + LINADDR_PTI(virt);

	if ((*pte & PAGE_PRESENT)) {
		if (TABLE_ENTRY_ADDR(*pte) == phys) {
			spin_release_irqrestore(&paging_lock, prev_irq);
			return;
		}
		LOG(__FUNCTION__, "Remapping virtual address %p to %p (previously mapped to %p)", virt, phys, TABLE_ENTRY_ADDR(*pte));
	} else {
		inc_page(TABLE_ENTRY_ADDR(*pde));
	}
	*pte = phys | PAGE_PRESENT | flags;
	inc_page(phys);
	invlpg(virt);

	spin_release_irqrestore(&paging_lock, prev_irq);
}

void unmap_page(const void *virt_addr) {
	uintptr_t virt = (uintptr_t)virt_addr & -0x1000LL;

	uint64_t flags = spin_acquire_irqsave(&paging_lock);

	uint64_t *pml4e = (uint64_t *)LINADDR_PML4E_PTR(virt);
	if ((*pml4e & PAGE_PRESENT) == 0)
		goto unmapped;
	uint64_t *pdpte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pml4e)) + LINADDR_PDPTI(virt);
	if ((*pdpte & PAGE_PRESENT) == 0)
		goto unmapped;
	uint64_t *pde = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pdpte)) + LINADDR_PDI(virt);
	if ((*pde & PAGE_PRESENT) == 0)
		goto unmapped;
	uint64_t *pte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pde)) + LINADDR_PTI(virt);
	if ((*pte & PAGE_PRESENT) == 0)
		goto unmapped;

	if (dec_page(TABLE_ENTRY_ADDR(*pte)) > 0) {
		spin_release_irqrestore(&paging_lock, flags);
		return;
	}
	*pte = 0;
	invlpg(virt);

	if (dec_page(TABLE_ENTRY_ADDR(*pde)) > 0) {
		spin_release_irqrestore(&paging_lock, flags);
		return;
	}
	free_page((void *)TABLE_ENTRY_ADDR(*pde));
	*pde = 0;
	invlpg(pte);

	if (dec_page(TABLE_ENTRY_ADDR(*pdpte)) > 0) {
		spin_release_irqrestore(&paging_lock, flags);
		return;
	}
	free_page((void *)TABLE_ENTRY_ADDR(*pdpte));
	*pdpte = 0;
	invlpg(pde);

	if (dec_page(TABLE_ENTRY_ADDR(*pml4e)) > 0) {
		spin_release_irqrestore(&paging_lock, flags);
		return;
	}
	free_page((void *)TABLE_ENTRY_ADDR(*pml4e));
	*pml4e = 0;
	invlpg(pdpte);
	spin_release_irqrestore(&paging_lock, flags);
	return;

unmapped:
	spin_release_irqrestore(&paging_lock, flags);
	LOG(__FUNCTION__, "Attempted to unmap non-mapped page at %p", virt);
}

bool is_mapped(const void *virt_addr) {
	uintptr_t virt = (uintptr_t)virt_addr & -0x1000LL;

	uint64_t flags = spin_acquire_irqsave(&paging_lock);

	uint64_t *pml4e = (uint64_t *)LINADDR_PML4E_PTR(virt);
	if ((*pml4e & PAGE_PRESENT) == 0) {
		spin_release_irqrestore(&paging_lock, flags);
		return false;
	}

	uint64_t *pdpte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pml4e)) + LINADDR_PDPTI(virt);
	if ((*pdpte & PAGE_PRESENT) == 0) {
		spin_release_irqrestore(&paging_lock, flags);
		return false;
	}
	if (*pdpte & PAGE_PS) {
		spin_release_irqrestore(&paging_lock, flags);
		return true;
	}

	uint64_t *pde = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pdpte)) + LINADDR_PDI(virt);
	if ((*pde & PAGE_PRESENT) == 0) {
		spin_release_irqrestore(&paging_lock, flags);
		return false;
	}
	if (*pde & PAGE_PS) {
		spin_release_irqrestore(&paging_lock, flags);
		return true;
	}

	uint64_t *pte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pde)) + LINADDR_PTI(virt);
	spin_release_irqrestore(&paging_lock, flags);
	return *pte & PAGE_PRESENT;
}

int inc_page(const physaddr_t phys) {
	pageinfo_t *info = get_pageinfo(phys);
	if (info == NULL)
		return -1;

	return atomic_fetch_add_explicit(&info->refcount, 1, memory_order_relaxed) + 1;
}

int dec_page(const physaddr_t phys) {
	pageinfo_t *info = get_pageinfo(phys);
	if (info == NULL)
		return -1;

	uint32_t prev_count = atomic_fetch_sub_explicit(&info->refcount, 1, memory_order_relaxed);
	if (prev_count == 0)
		panic("Invalid page decrement %p", (void *)phys);

	return prev_count - 1;
}
