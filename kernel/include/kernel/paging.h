#pragma once
#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#include <kernel/defines.h>

#include_next <paging.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define __pa(virt) ((uintptr_t)(virt) - hhdm_off)
#define __va(phys) ((void *)((uintptr_t)(phys) + hhdm_off))
#define PHYSADDR_NULL ((physaddr_t)(-1))

extern uintptr_t hhdm_off;

void map_page(const void *virt, const physaddr_t phys, uint64_t flags);
void unmap_page(const void *virt);
bool is_mapped(const void *virt);

int32_t inc_page(const physaddr_t phys);
int32_t dec_page(const physaddr_t phys);

#endif
