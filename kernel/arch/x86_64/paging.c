#include <kernel/paging.h>

#include <stdio.h>
#include <string.h>

#include <kernel/log.h>
#include <kernel/mman.h>
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
		panic("Page fault at address %p RIP %p, no VMA found", (void *)addr, frame->isr_rip);

	// Right now we only handle demand paging, so we will allocate a new page
	/// TODO: Handle other cases (CoW, file mappings, etc.)
	/// TODO: Handle flags properly
	map_page((void *)addr, alloc_page(), PAGE_PRESENT | PAGE_RW | PAGE_NX);

	return frame;
}

static spinlock_t paging_lock = SPINLOCK_INITIALIZER;

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
	pdpte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pml4e)) + LINADDR_PDPTE(virt);

	if ((*pdpte & PAGE_PRESENT) == 0) {
		physaddr_t addr = alloc_page();
		*pdpte = addr | PAGE_PRESENT | PAGE_RW;
		memset((void *)__va(addr), 0, 0x1000);
		// Bits 52:61 store the number of entries in the referenced pdpt/pd/pt
		*pml4e += 1ULL << 52;
	}
	pde = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pdpte)) + LINADDR_PDE(virt);

	if ((*pde & PAGE_PRESENT) == 0) {
		physaddr_t addr = alloc_page();
		*pde = addr | PAGE_PRESENT | PAGE_RW;
		memset((void *)__va(addr), 0, 0x1000);
		*pdpte += 1ULL << 52;
	}
	pte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pde)) + LINADDR_PTE(virt);

	if ((*pte & PAGE_PRESENT)) {
		if (TABLE_ENTRY_ADDR(*pte) == phys) {
			spin_release_irqrestore(&paging_lock, prev_irq);
			return;
		}
		LOG(__FUNCTION__, "Remapping virtual address %p to %p (previously mapped to %p)", virt, phys, TABLE_ENTRY_ADDR(*pte));
	} else {
		*pde += 1ULL << 52;
	}
	invlpg(virt);
	*pte = phys | PAGE_PRESENT | flags;

	spin_release_irqrestore(&paging_lock, prev_irq);
}

void unmap_page(const void *virt_addr) {
	uintptr_t virt = (uintptr_t)virt_addr & -0x1000LL;

	uint64_t flags = spin_acquire_irqsave(&paging_lock);

	uint64_t *pml4e = (uint64_t *)LINADDR_PML4E_PTR(virt);
	if ((*pml4e & PAGE_PRESENT) == 0) {
		spin_release_irqrestore(&paging_lock, flags);
		goto unmapped;
	}
	uint64_t *pdpte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pml4e)) + LINADDR_PDPTE(virt);
	if ((*pdpte & PAGE_PRESENT) == 0) {
		spin_release_irqrestore(&paging_lock, flags);
		goto unmapped;
	}
	uint64_t *pde = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pdpte)) + LINADDR_PDE(virt);
	if ((*pde & PAGE_PRESENT) == 0) {
		spin_release_irqrestore(&paging_lock, flags);
		goto unmapped;
	}
	uint64_t *pte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pde)) + LINADDR_PTE(virt);
	if ((*pte & PAGE_PRESENT) == 0) {
		spin_release_irqrestore(&paging_lock, flags);
		goto unmapped;
	}

	*pte = 0;
	invlpg(virt);

	if (((*pde -= (1ULL << 52)) & (0x1ffULL << 52)) != 0) {
		spin_release_irqrestore(&paging_lock, flags);
		return;
	}
	free_page((void *)TABLE_ENTRY_ADDR(*pde));
	*pde = 0;
	invlpg(pte);

	if (((*pdpte -= (1ULL << 52)) & (0x1ffULL << 52)) != 0) {
		spin_release_irqrestore(&paging_lock, flags);
		return;
	}
	free_page((void *)TABLE_ENTRY_ADDR(*pdpte));
	*pdpte = 0;
	invlpg(pde);

	if (((*pml4e -= (1ULL << 52)) & (0x1ffULL << 52)) != 0) {
		spin_release_irqrestore(&paging_lock, flags);
		return;
	}
	free_page((void *)TABLE_ENTRY_ADDR(*pml4e));
	*pml4e = 0;
	invlpg(pdpte);
	spin_release_irqrestore(&paging_lock, flags);
	return;

unmapped:
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

	uint64_t *pdpte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pml4e));
	if ((*pdpte & PAGE_PRESENT) == 0) {
		spin_release_irqrestore(&paging_lock, flags);
		return false;
	}
	if (*pdpte & PAGE_PS) {
		spin_release_irqrestore(&paging_lock, flags);
		return true;
	}

	uint64_t *pde = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pdpte));
	if ((*pde & PAGE_PRESENT) == 0) {
		spin_release_irqrestore(&paging_lock, flags);
		return false;
	}
	if (*pde & PAGE_PS) {
		spin_release_irqrestore(&paging_lock, flags);
		return true;
	}

	uint64_t *pte = (uint64_t *)__va(TABLE_ENTRY_ADDR(*pde));
	spin_release_irqrestore(&paging_lock, flags);
	return *pte & PAGE_PRESENT;
}
