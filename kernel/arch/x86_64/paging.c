#include <kernel/paging.h>

#include <stdio.h>
#include <string.h>

#include <kernel/log.h>
#include <kernel/mman.h>
#include <kernel/panic.h>

extern uintptr_t hhdm_off;

void map_page(const void *virt_addr, const void *phys_addr, uint64_t flags) {
	uintptr_t phys = (uintptr_t)phys_addr & -0x1000LL;
	uintptr_t virt = (uintptr_t)virt_addr & -0x1000LL;
	if (virt_addr == NULL)
		panic("map_page: No virtual address provided");

	uint64_t *pml4e = (uint64_t *)LINADDR_PML4E_PTR(virt);
	uint64_t *pdpte;
	uint64_t *pde;
	uint64_t *pte;

	if ((*pml4e & PAGE_PRESENT) == 0) {
		uintptr_t addr = (uintptr_t)alloc_page();
		*pml4e = addr | PAGE_PRESENT | PAGE_RW;
		memset((void *)(hhdm_off + addr), 0, 0x1000);
	}
	pdpte = (uint64_t *)(hhdm_off + TABLE_ENTRY_ADDR(*pml4e)) + LINADDR_PDPTE(virt);

	if ((*pdpte & PAGE_PRESENT) == 0) {
		uintptr_t addr = (uintptr_t)alloc_page();
		*pdpte = addr | PAGE_PRESENT | PAGE_RW;
		memset((void *)(hhdm_off + addr), 0, 0x1000);
		// Bits 52:61 store the number of entries in the referenced pdpt/pd/pt
		*pml4e += 1ULL << 52;
	}
	pde = (uint64_t *)(hhdm_off + TABLE_ENTRY_ADDR(*pdpte)) + LINADDR_PDE(virt);

	if ((*pde & PAGE_PRESENT) == 0) {
		uintptr_t addr = (uintptr_t)alloc_page();
		*pde = addr | PAGE_PRESENT | PAGE_RW;
		memset((void *)(hhdm_off + addr), 0, 0x1000);
		*pdpte += 1ULL << 52;
	}
	pte = (uint64_t *)(hhdm_off + TABLE_ENTRY_ADDR(*pde)) + LINADDR_PTE(virt);

	if ((*pte & PAGE_PRESENT)) {
		if (TABLE_ENTRY_ADDR(*pte) == phys)
			return;
		LOG(__FUNCTION__, "Remapping virtual address %p to %p (previously mapped to %p)", virt, phys, TABLE_ENTRY_ADDR(*pte));
	} else {
		*pde += 1ULL << 52;
	}
	invlpg(virt);
	*pte = phys | PAGE_PRESENT | flags;
}

void unmap_page(const void *virt_addr) {
	uintptr_t virt = (uintptr_t)virt_addr & -0x1000LL;

	uint64_t *pml4e = (uint64_t *)LINADDR_PML4E_PTR(virt);
	if ((*pml4e & PAGE_PRESENT) == 0)
		goto unmapped;
	uint64_t *pdpte = (uint64_t *)(hhdm_off + TABLE_ENTRY_ADDR(*pml4e));
	if ((*pdpte & PAGE_PRESENT) == 0)
		goto unmapped;
	uint64_t *pde = (uint64_t *)(hhdm_off + TABLE_ENTRY_ADDR(*pdpte));
	if ((*pde & PAGE_PRESENT) == 0)
		goto unmapped;
	uint64_t *pte = (uint64_t *)(hhdm_off + TABLE_ENTRY_ADDR(*pde));
	if ((*pte & PAGE_PRESENT) == 0)
		goto unmapped;

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
	return;

unmapped:
	LOG(__FUNCTION__, "Attempted to unmap non-mapped page at %p", virt);
}

bool is_mapped(const void *virt_addr) {
	uintptr_t virt = (uintptr_t)virt_addr & -0x1000LL;

	uint64_t *pml4e = (uint64_t *)LINADDR_PML4E_PTR(virt);
	if ((*pml4e & PAGE_PRESENT) == 0)
		return false;

	uint64_t *pdpte = (uint64_t *)(hhdm_off + TABLE_ENTRY_ADDR(*pml4e));
	if ((*pdpte & PAGE_PRESENT) == 0)
		return false;
	if (*pdpte & PAGE_PS)
		return true;

	uint64_t *pde = (uint64_t *)(hhdm_off + TABLE_ENTRY_ADDR(*pdpte));
	if ((*pde & PAGE_PRESENT) == 0)
		return false;
	if (*pde & PAGE_PS)
		return true;

	uint64_t *pte = (uint64_t *)(hhdm_off + TABLE_ENTRY_ADDR(*pde));
	return *pte & PAGE_PRESENT;
}
