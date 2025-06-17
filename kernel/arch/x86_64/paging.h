#pragma once
#ifndef ARCH_X86_64_PAGING_H
#define ARCH_X86_64_PAGING_H

#ifndef __arch_x86_64__
#error "This file should only be included on x86_64 architecture."
#endif

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
#define LINADDR_PML4(addr) (((addr) >> 39) & 0x1ff)
#define LINADDR_PDPT(addr) (((addr) >> 30) & 0x1ff)
#define LINADDR_PD(addr) (((addr) >> 21) & 0x1ff)
#define LINADDR_PT(addr) (((addr) >> 12) & 0x1ff)
#define TABLE_ENTRY_ADDR(entry) ((entry) & 0x07fffffffffff000)

#endif
