#include <kernel/vmem.h>

#include <stdio.h>

#include <kernel/mman.h>
#include <kernel/panic.h>

void map_page(const void *virt_addr, const void *phys_addr, uint64_t flags) {
	uintptr_t phys = (uintptr_t)phys_addr & -0x1000LL;
	uintptr_t virt = (uintptr_t)virt_addr & -0x1000LL;
	if (virt_addr == NULL)
		panic("map_page: No virtual address provided");

	uint64_t *pml4e = (uint64_t *)LINADDR_PML4E_PTR(virt);
	if ((*pml4e & PAGE_PRESENT) == 0) {
		*pml4e = (uintptr_t)kpalloc_one() | PAGE_PRESENT | PAGE_RW;
	}

	uint64_t *pdpte = (uint64_t *)LINADDR_PDPTE_PTR(virt);
	if ((*pdpte & PAGE_PRESENT) == 0) {
		*pdpte = (uintptr_t)kpalloc_one() | PAGE_PRESENT | PAGE_RW;
		// Bits 52:61 store the number of entries in the referenced pdpt/pd/pt
		*pml4e += 1ULL << 52;
	}

	uint64_t *pde = (uint64_t *)LINADDR_PDE_PTR(virt);
	if ((*pde & PAGE_PRESENT) == 0) {
		*pde = (uintptr_t)kpalloc_one() | PAGE_PRESENT | PAGE_RW;
		*pdpte += 1ULL << 52;
	}

	uint64_t *pte = (uint64_t *)LINADDR_PTE_PTR(virt);
	if ((*pte & PAGE_PRESENT) && TABLE_ENTRY_ADDR(*pte) != phys) {
		printf("map_page: Remapping virtual address %p to %p (previously mapped to %p)\n", virt, phys, TABLE_ENTRY_ADDR(*pte));
	} else {
		*pde += 1ULL << 52;
	}
	invlpg(virt);
	*pte = phys | PAGE_PRESENT | flags;
}

void unmap_page(const void *virt_addr) {
	uintptr_t virt = (uintptr_t)virt_addr & -0x1000LL;

	uint64_t *pte = (uint64_t *)LINADDR_PML4E_PTR(virt);
	if ((*pte & PAGE_PRESENT) == 0)
		panic("unmap_page: Attempted to unmap non-mapped page0");
	*pte = 0;
	invlpg(virt);

	uint64_t *pde = (uint64_t *)LINADDR_PDPTE_PTR(virt);
	if (((*pde -= (1ULL << 52)) & (0x1ffULL << 52)) != 0)
		return;
	kpfree_one((void *)TABLE_ENTRY_ADDR(*pde));
	*pde = 0;

	uint64_t *pdpte = (uint64_t *)LINADDR_PDE_PTR(virt);
	if (((*pdpte -= (1ULL << 52)) & (0x1ffULL << 52)) != 0)
		return;
	kpfree_one((void *)TABLE_ENTRY_ADDR(*pdpte));
	*pdpte = 0;

	uint64_t *pml4e = (uint64_t *)LINADDR_PTE_PTR(virt);
	if (((*pml4e -= (1ULL << 52)) & (0x1ffULL << 52)) != 0)
		return;
	kpfree_one((void *)TABLE_ENTRY_ADDR(*pml4e));
	*pml4e = 0;
}
