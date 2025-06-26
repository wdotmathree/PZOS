#include <kernel/paging.h>

#include <stdio.h>
#include <string.h>

#include <kernel/mman.h>
#include <kernel/paging.h>
#include <kernel/panic.h>

void map_page(const void *virt_addr, const void *phys_addr, uint64_t flags) {
	uintptr_t phys = (uintptr_t)phys_addr & -0x1000LL;
	uintptr_t virt = (uintptr_t)virt_addr & -0x1000LL;
	if (virt_addr == NULL)
		panic("map_page: No virtual address provided");

	uint64_t *pml4e = (uint64_t *)LINADDR_PML4E_PTR(virt);
	uint64_t *pdpte = (uint64_t *)LINADDR_PDPTE_PTR(virt);
	uint64_t *pde = (uint64_t *)LINADDR_PDE_PTR(virt);
	uint64_t *pte = (uint64_t *)LINADDR_PTE_PTR(virt);

	if ((*pml4e & PAGE_PRESENT) == 0) {
		uintptr_t addr = (uintptr_t)alloc_page();
		*pml4e = addr | PAGE_PRESENT | PAGE_RW;
		invlpg(pdpte);
		memset((void *)((uintptr_t)pdpte & -0x1000), 0, 0x1000);
	}

	if ((*pdpte & PAGE_PRESENT) == 0) {
		uintptr_t addr = (uintptr_t)alloc_page();
		*pdpte = addr | PAGE_PRESENT | PAGE_RW;
		invlpg(pde);
		memset((void *)((uintptr_t)pde & -0x1000), 0, 0x1000);
		// Bits 52:61 store the number of entries in the referenced pdpt/pd/pt
		*pml4e += 1ULL << 52;
	}

	if ((*pde & PAGE_PRESENT) == 0) {
		uintptr_t addr = (uintptr_t)alloc_page();
		*pde = addr | PAGE_PRESENT | PAGE_RW;
		invlpg(pte);
		memset((void *)((uintptr_t)pte & -0x1000), 0, 0x1000);
		*pdpte += 1ULL << 52;
	}

	if ((*pte & PAGE_PRESENT)) {
		if (TABLE_ENTRY_ADDR(*pte) == phys)
			return;
		printf("map_page: Remapping virtual address %p to %p (previously mapped to %p)\n", virt, phys, TABLE_ENTRY_ADDR(*pte));
	} else {
		*pde += 1ULL << 52;
	}
	invlpg(virt);
	*pte = phys | PAGE_PRESENT | flags;
}

void unmap_page(const void *virt_addr) {
	uintptr_t virt = (uintptr_t)virt_addr & -0x1000LL;
	if (!is_mapped(virt_addr)) {
		printf("unmap_page: Attempted to unmap non-mapped page at %p\n", virt);
		return;
	}

	uint64_t *pml4e = (uint64_t *)LINADDR_PML4E_PTR(virt);
	uint64_t *pdpte = (uint64_t *)LINADDR_PDPTE_PTR(virt);
	uint64_t *pde = (uint64_t *)LINADDR_PDE_PTR(virt);
	uint64_t *pte = (uint64_t *)LINADDR_PTE_PTR(virt);

	*pte = 0;
	invlpg(virt);

	if (((*pde -= (1ULL << 52)) & (0x1ffULL << 52)) != 0)
		return;
	free_page((void *)TABLE_ENTRY_ADDR(*pde));
	*pde = 0;
	invlpg(pte);

	if (((*pdpte -= (1ULL << 52)) & (0x1ffULL << 52)) != 0)
		return;
	free_page((void *)TABLE_ENTRY_ADDR(*pdpte));
	*pdpte = 0;
	invlpg(pde);

	if (((*pml4e -= (1ULL << 52)) & (0x1ffULL << 52)) != 0)
		return;
	free_page((void *)TABLE_ENTRY_ADDR(*pml4e));
	*pml4e = 0;
	invlpg(pdpte);
}

bool is_mapped(const void *virt_addr) {
	uintptr_t virt = (uintptr_t)virt_addr & -0x1000LL;

	uint64_t *pml4e = (uint64_t *)LINADDR_PML4E_PTR(virt);
	uint64_t *pdpte = (uint64_t *)LINADDR_PDPTE_PTR(virt);
	uint64_t *pde = (uint64_t *)LINADDR_PDE_PTR(virt);
	uint64_t *pte = (uint64_t *)LINADDR_PTE_PTR(virt);

	return (*pml4e & PAGE_PRESENT) && (*pdpte & PAGE_PRESENT) && (*pde & PAGE_PRESENT) && (*pte & PAGE_PRESENT);
}
