#pragma once
#ifndef KERNEL_ACPI_H
#define KERNEL_ACPI_H

#include <kernel/efi.h>

typedef uint32_t acpi_sig_t;

#define ACPI_SIG(s) (*(acpi_sig_t *)(s))

// These probably don't exist anymore so this is just here for size calculations
typedef struct {
	char Signature[8];
	uint8_t Checksum;
	char OEMID[6];
	uint8_t Revision;
	uint32_t RsdtAddress;
} __attribute__((packed)) ACPI_RSDP;

typedef struct {
	char Signature[8];
	uint8_t Checksum;
	char OEMID[6];
	uint8_t Revision;
	uint32_t RsdtAddress; // deprecated since version 2.0

	uint32_t Length;
	uint64_t XsdtAddress;
	uint8_t ExtendedChecksum;
	uint8_t reserved[3];
} __attribute__((packed)) ACPI_XSDP;

typedef struct {
	char Signature[4];
	uint32_t Length;
	uint8_t Revision;
	uint8_t Checksum;
	char OEMID[6];
	char OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
} ACPI_TABLE_HEADER;

// If the system table is NULL, it will probe low memory
void acpi_init(EFI_SYSTEM_TABLE *system_table);

void *acpi_get_table(acpi_sig_t signature);

#endif
