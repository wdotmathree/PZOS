#pragma once
#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#include_next <paging.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define __pa(virt) ((uintptr_t)(virt) - hhdm_off)
#define __va(phys) ((void *)((phys) + hhdm_off))
#define PHYSADDR_NULL ((physaddr_t)(-1))

typedef uintptr_t physaddr_t;

extern uintptr_t hhdm_off;

void map_page(const void *virt, const physaddr_t phys, uint64_t flags);
void unmap_page(const void *virt);
bool is_mapped(const void *virt_addr);

#endif
