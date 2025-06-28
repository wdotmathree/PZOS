/**
 * Virtual Map: (Below are PAGE NUMBERS, not addresses)
 * PML4[0x100-0x17f] 0x800000000-0xbffffffff: Reserved
 * PML4[0x180] 0xc00000000: Kernel heap - Can expand up to 8TiB (16 PML4 entries)
 * PML4[0x1a0] 0xd00000000: Kernel virtual area - Can expand up to 8TiB (16 PML4 entries)
 * PML4[0x1c0] 0xe00000000: I/O mappings - Can expand up to 8TiB (16 PML4 entries)
 * PML4[0x1f0] 0xf80000000: Framebuffer (Mapped using large pages if possible)
 * PML4[0x1fe] 0xff0000000: Recursive mapping (Second last PML4 entry)
 * PML4[0x1ff] 0xff8000000: Kernel execution space
 * 		L--> (0xff8000000-0xfffefffff): Memory management information (bitmaps, stacks, etc.)
 * 		L--> (0xffff00000-0xffff7ffff): Kernel stacks (Only top page is mapped by mman_init())
 * 		L--> (0xffff80000-0xfffffxxxx): Kernel executable (Segments as loaded by Limine)
 *
 * Notes:
 * - Mappings are initially set up in mman_init(), where we replace Limine's default HHDM
 * - Spaces between specified sections are guard spaces, which are NEVER mapped
 */

#pragma once
#ifndef KERNEL_VMEM_H
#define KERNEL_VMEM_H

#include <stddef.h>
#include <stdint.h>

#define VMEM_HEAP_BASE 0xffffc00000000000
#define VMEM_VIRT_BASE 0xffffd00000000000
#define VMEM_MMIO_BASE 0xffffe00000000000
#define VMEM_STACK_BASE 0xffffffff80000000

#define VMEM_HEAP_END 0xffffc80000000000
#define VMEM_VIRT_END 0xffffd80000000000
#define VMEM_MMIO_END 0xffffe80000000000

#define KERNEL_STACK_SIZE (64 * 1024) // 64 KiB stack size

#define VMA_READ 0x1
#define VMA_WRITE 0x2
#define VMA_EXEC 0x4

#define PF_PRESENT 0x1
#define PF_WRITE 0x2
#define PF_USER 0x4
#define PF_RESERVED 0x8
#define PF_NX 0x10

struct vma {
	void *base;
	size_t size;
	uint64_t flags;

	struct vma *next;
	struct vma *prev;
};

void vmem_init(void);
void create_vma(void *base, size_t size, uint64_t flags);
void destroy_vma(struct vma *vma);

// Allocates `npages` pages of virtual memory, initially not backed by anything
void *vmalloc(size_t npages, uint64_t flags);
void *vmalloc_at(void *base, void *limit, size_t npages, uint64_t flags);

#endif
