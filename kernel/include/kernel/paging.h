#pragma once
#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#include_next <paging.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern uintptr_t hhdm_off;

void map_page(const void *virt, const void *phys, uint64_t flags);
void unmap_page(const void *virt);
bool is_mapped(const void *virt_addr);

#endif
