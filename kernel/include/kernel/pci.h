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

typedef struct {
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t class_code;
	uint8_t subclass;
	uint8_t prog_if;
	uint8_t revision_id;
	uint8_t header_type;
} __attribute__((packed)) pci_header_t;

typedef struct pci_dev_t {
	struct pci_dev_t *sibling;
	struct pci_dev_t *next;
	uint16_t seg_group;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	// pci_header_t *header;
} pci_dev_t;

typedef struct pci_bus_t {
	struct pci_bus_t *parent;
	struct pci_bus_t *children;
	struct pci_bus_t *next;
	struct pci_dev_t *bridge;
	pci_dev_t *devices;
	uint8_t bus;
} pci_bus_t;

void pci_init(void);

#endif
