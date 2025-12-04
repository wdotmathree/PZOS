/**
 * Virtual Map:
 * PML4[0x100-0x17f] 0xffff800000000000-0xffffbfffffffffff: Direct map (64 TiB)
 * PML4[0x190-0x1cf] 0xffffc80000000000-0xffffe7ffffffffff: Kernel virtual area (32 TiB)
 * PML4[0x1fd-0x1fd] 0xfffffe8000000000-0xfffffeffffffffff: Framebuffer (512 GiB)
 * PML4[0x1fe-0x1fe] 0xffffff0000000000-0xffffff7fffffffff: Recursive page table mapping (512 GiB)
 * PML4[0x1ff-0x1ff]: +--> 0xffffffff10000000-0xffffffff7fffffff: Kernel modules (1.75 GiB)
 *					  +--> 0xffffffff80000000-0xffffffff9fffffff: Kernel executable (512 MiB)
 *					  +--> 0xffffffffa0000000-0xffffffffffffefff: Kernel stacks (~1.25 GiB)
 *					  \--> 0xfffffffffffff000-0xffffffffffffffff: BSP stack guard page
 * Notes:
 * - If KASLR is enabled, the direct map and virtual area are shifted (independently) by a random offset
 * - Mappings are initially set up in mman_init()
 * - The above areas will always have at least one guard page between them, regardless of KASLR offsets
 * - The virtual area is used for vmalloc and I/O mappings
 */

#pragma once
#ifndef KERNEL_VMEM_H
#define KERNEL_VMEM_H

#include <kernel/paging.h>

#include <stddef.h>
#include <stdint.h>

#define VMEM_VIRT_BASE 0xffffc80000000000
#define VMEM_VIRT_END 0xffffe80000000000
#define VMEM_STACK_BASE 0xfffffffffffff000

#define KERNEL_STACK_SIZE (64 * 1024) // 64 KiB stack size

#define VMA_READ 0x1
#define VMA_WRITE 0x2
#define VMA_EXEC 0x4

#define PF_PRESENT 0x1
#define PF_WRITE 0x2
#define PF_USER 0x4
#define PF_RESERVED 0x8
#define PF_NX 0x10

typedef struct vma_list_t {
	void *base;
	size_t size;
	uint64_t flags;

	struct vma_list_t *next;
	struct vma_list_t *prev;
} vma_list_t;

void vmem_init(void);
void create_vma(void *base, size_t size, uint64_t flags);
void destroy_vma(vma_list_t *vma);

// Allocates `npages` pages of demand paged virtual memory
void *vmalloc(size_t npages, uint64_t flags);
void *vmalloc_at(void *base, void *limit, size_t npages, uint64_t flags);

// Frees vmalloc'd memory starting at the page referenced by `addr`
// All `npages` must belong to the same VMA
void vfree(void *addr, size_t npages);

#endif
