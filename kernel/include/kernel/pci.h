#pragma once
#ifndef KERNEL_PCI_H
#define KERNEL_PCI_H

#include <stdint.h>

#include <kernel/acpi.h>

typedef struct {
	uintptr_t base_address;
	uint16_t segment_group_number;
	uint8_t start_bus_number;
	uint8_t end_bus_number;
	char _reserved[4];
} __attribute__((packed)) ACPI_MCFG_ENTRY;

typedef struct {
	ACPI_TABLE_HEADER Header;
	char _reserved[8];
	ACPI_MCFG_ENTRY entries[];
} __attribute__((packed)) ACPI_MCFG;

void pci_init(void);

#endif
