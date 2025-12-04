#include <kernel/kmalloc.h>

#include <kernel/paging.h>
#include <kernel/panic.h>
#include <kernel/spinlock.h>
#include <kernel/vmem.h>

void *kmalloc(size_t size) {
	vmalloc((size + PAGE_SIZE - 1) / PAGE_SIZE, VMA_READ | VMA_WRITE);
}

void kfree(void *ptr) {
	vfree(ptr, 1);
}
