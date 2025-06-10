#pragma once
#ifndef KERNEL_PMMAN_H
#define KERNEL_PMMAN_H

#include <limine.h>
#include <stddef.h>

void pmman_init(struct limine_memmap_response *mmap, intptr_t hhdm_off, intptr_t kernel_base, intptr_t kernel_size);

#endif
