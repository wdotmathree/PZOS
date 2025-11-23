#pragma once
#ifndef KERNEL_PCI_H
#define KERNEL_PCI_H

#include <stdbool.h>
#include <stdint.h>

#include <kernel/acpi.h>

#define PCI_ID_ANY (-1)

#define PCI_CMD_IO_SPACE 0x01
#define PCI_CMD_MEM_SPACE 0x02
#define PCI_CMD_BUS_MASTER 0x04
#define PCI_CMD_SPECIAL_CYCLES 0x08
#define PCI_CMD_MEM_WRITE_INVALIDATE 0x10
#define PCI_CMD_VGA_PALETTE_SNOOP 0x20
#define PCI_CMD_PARITY_ERROR_RESP 0x40
#define PCI_CMD_SERR_ENABLE 0x100
#define PCI_CMD_FAST_B2B_ENABLE 0x200
#define PCI_CMD_INTERRUPT_DISABLE 0x400

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
	uint16_t command;
	uint16_t status;
	uint8_t revision_id;
	uint8_t prog_if;
	uint8_t subclass;
	uint8_t class_code;
	uint8_t cl_size;
	uint8_t latency_timer;
	uint8_t header_type;
	uint8_t bist;
} __attribute__((packed)) pci_header_t;

typedef struct {
	pci_header_t common;
	uint32_t bar0;
	uint32_t bar1;
	uint32_t bar2;
	uint32_t bar3;
	uint32_t bar4;
	uint32_t bar5;
	uint32_t cis_pointer;
	uint16_t sub_vendor_id;
	uint16_t sub_id;
	uint32_t rom_base_addr;
	uint8_t cap_ptr;
	uint8_t _reserved[7];
	uint8_t irq_line;
	uint8_t irq_pin;
	uint8_t min_grant;
	uint8_t max_latency;
} __attribute__((packed)) pci_dev_header_t;

typedef struct pci_dev_t {
	struct pci_dev_t *sibling;
	struct pci_dev_t *next;
	uint16_t seg_group;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	pci_header_t *header;
	struct pci_driver_t *driver;
} pci_dev_t;

typedef struct pci_bus_t {
	struct pci_bus_t *parent;
	struct pci_bus_t *children;
	struct pci_bus_t *next;
	struct pci_dev_t *bridge;
	pci_dev_t *devices;
	uint8_t bus;
} pci_bus_t;

typedef struct pci_driver_t {
	struct pci_driver_t *next;
	uint32_t class_code, class_mask;
	uint32_t vendor_id, device_id;
	bool (*attach)(pci_dev_t *dev); // Returns true to claim device
} pci_driver_t;

void pci_init(void);

bool pci_register_driver(pci_driver_t *driver);

#endif
