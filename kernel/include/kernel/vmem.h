#pragma once
#ifndef KERNEL_VMEM_H
#define KERNEL_VMEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void map_page(const void *virt, const void *phys, uint64_t flags);
void unmap_page(const void *virt);
bool is_mapped(const void *virt_addr);

#endif
