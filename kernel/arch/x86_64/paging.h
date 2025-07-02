#pragma once
#ifndef ARCH_X86_64_PAGING_H
#define ARCH_X86_64_PAGING_H

#ifndef __arch_x86_64__
#error "This file should only be included on x86_64 architecture."
#endif

#define PAGE_SIZE 0x1000

// PAT memory types
#define PAT_WB 0
#define PAT_WT 1
#define PAT_UCM 2
#define PAT_UC 3
#define PAT_WP 4
#define PAT_WC 5

// Page flags
#define PAGE_PRESENT 0x1
#define PAGE_RW 0x2
#define PAGE_USER 0x4
#define PAGE_PWT 0x8 // Page write-through
#define PAGE_PCD 0x10 // Page cache disable
#define PAGE_ACCESSED 0x20
#define PAGE_DIRTY 0x40
#define PAGE_PS 0x80 // Large page size (2MB or 1GB)
#define PAGE_GLOBAL 0x100 // Ignore PCIDs when looking in TLB
#define PAGE_PAT 0x400 // Works together with PWT and PCD to lookup the memory type in the PAT
#define PAGE_NX 0x8000000000000000
#define PAGE_TYPE(x) (((((x) >> 2) & 1) * PAGE_PAT) | ((((x) >> 1) & 1) * PAGE_PCD) | (((x) & 1) * PAGE_PWT))

// Address conversion macros
#define LINADDR_PML4E(addr) (((addr) >> 39) & 0x1ff)
#define LINADDR_PDPTE(addr) (((addr) >> 30) & 0x1ff)
#define LINADDR_PDE(addr) (((addr) >> 21) & 0x1ff)
#define LINADDR_PTE(addr) (((addr) >> 12) & 0x1ff)
#define LINADDR_OFF(addr) ((addr) & 0xfff)
#define TABLE_ENTRY_ADDR(entry) ((entry) & 0x07fffffffffff000)
#define BUILD_LINADDR(pml4, pdpt, pd, pt, offset) \
	(((int64_t)(pml4) << (39 + 16) >> 16) | ((uint64_t)(pdpt) << 30) | ((uint64_t)(pd) << 21) | ((uint64_t)(pt) << 12) | (offset))
#define BUILD_PAGENUM(pml4, pdpt, pd, pt) \
	(((uint64_t)(pml4) << 27) | ((uint64_t)(pdpt) << 18) | ((uint64_t)(pd) << 9) | (pt))

#define LINADDR_PML4E_PTR(addr) ((uint64_t *)BUILD_LINADDR(0x1fe, 0x1fe, 0x1fe, 0x1fe, ((addr) >> 36) & 0xff8))
#define LINADDR_PDPTE_PTR(addr) ((uint64_t *)BUILD_LINADDR(0x1fe, 0x1fe, 0x1fe, 0, ((addr) >> 27) & 0x1ffff8))
#define LINADDR_PDE_PTR(addr) ((uint64_t *)BUILD_LINADDR(0x1fe, 0x1fe, 0, 0, ((addr) >> 18) & 0x3ffffff8))
#define LINADDR_PTE_PTR(addr) ((uint64_t *)BUILD_LINADDR(0x1fe, 0, 0, 0, ((addr) >> 9) & 0x7ffffffff8))

#define invlpg(addr) \
	asm volatile("invlpg [%0]" : : "r"((void *)(addr)) : "memory")
#define invltlb()                   \
	asm volatile("mov rax, cr3\n\t" \
				 "mov cr3, rax"     \
				 : : : "rax", "memory")
#define invltlb_all()               \
	asm volatile("mov rax, cr0\n\t" \
				 "mov cr0, rax"     \
				 : : : "rax", "memory")

#endif
