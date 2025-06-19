#include <kernel/vmem.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <kernel/mman.h>
#include <kernel/paging.h>

void *map_page(const void *phys_addr, const void *virt_addr, uint64_t flags) {
	uintptr_t phys = (uintptr_t)phys_addr;
	uintptr_t virt = (uintptr_t)virt_addr;

	if (virt_addr == NULL) {
		/// TODO: Assign an arbitrary virtual address (decide on the scheme)
	}

	uintptr_t pml4 = LINADDR_PML4E(virt);
	uintptr_t pdpt = LINADDR_PDPTE(virt);
	uintptr_t pd = LINADDR_PDE(virt);
	uintptr_t pt = LINADDR_PTE(virt);

	uint64_t *pml4e = (uint64_t *)BUILD_LINADDR(0x1fe, 0x1fe, 0x1fe, 0x1fe, pml4);
	if ((*pml4e & PAGE_PRESENT) == 0) {
		*pml4e = (uintptr_t)kpalloc_one() | PAGE_PRESENT | PAGE_RW;
	}

	uint64_t *pdpte = (uint64_t *)BUILD_LINADDR(0x1fe, 0x1fe, 0x1fe, pml4, pdpt);
	if ((*pdpte & PAGE_PRESENT) == 0) {
		*pdpte = (uintptr_t)kpalloc_one() | PAGE_PRESENT | PAGE_RW;
	}

	uint64_t *pde = (uint64_t *)BUILD_LINADDR(0x1fe, 0x1fe, pml4, pdpt, pd);
	if ((*pde & PAGE_PRESENT) == 0) {
		*pde = (uintptr_t)kpalloc_one() | PAGE_PRESENT | PAGE_RW;
	}

	uint64_t *pte = (uint64_t *)BUILD_LINADDR(0x1fe, pml4, pdpt, pd, pt);
	if ((*pte & PAGE_PRESENT) && TABLE_ENTRY_ADDR(*pte) != phys) {
		printf("VMEM: Remapping virtual address %p to %p (previously mapped to %p)\n", virt, phys, TABLE_ENTRY_ADDR(*pte));
	}

	*pte = phys | PAGE_PRESENT | flags;
	invlpg(virt);

	return (void *)virt;
}

void unmap_page(const void *virt_addr) {
	uintptr_t virt = (uintptr_t)virt_addr;

	uintptr_t pml4 = LINADDR_PML4E(virt);
	uintptr_t pdpt = LINADDR_PDPTE(virt);
	uintptr_t pd = LINADDR_PDE(virt);
	uintptr_t pt = LINADDR_PTE(virt);

	uint64_t *pte = (uint64_t *)BUILD_LINADDR(0x1fe, pdpt, pd, pml4, pt);
	if (*pte & PAGE_PRESENT) {
		*pte = 0;
		invlpg(virt);
	}
}
