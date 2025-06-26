#include <kernel/vmem.h>

// struct vma {
// 	uintptr_t base;
// 	size_t size;
// 	uint64_t flags;
// };

// struct vma *create_vma(void *base, size_t size, uint64_t flags) {
// 	struct vma *vma = (struct vma *)alloc_page();
// 	if (!vma) {
// 		panic("create_vma: Failed to allocate memory for VMA");
// 	}
// 	vma->base = (uintptr_t)base;
// 	vma->size = size;
// 	vma->flags = flags;
// 	return vma;
// }
