#pragma once
#ifndef KERNEL_VMEM_H
#define KERNEL_VMEM_H

#include <stddef.h>
#include <stdint.h>

#include <kernel/paging.h>

void map_page(const void *virt, const void *phys, uint64_t flags);
void unmap_page(const void *virt);

#endif
