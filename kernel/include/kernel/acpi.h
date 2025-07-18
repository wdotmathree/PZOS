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

typedef struct {
	uint8_t AddressSpace;
	uint8_t BitWidth;
	uint8_t BitOffset;
	uint8_t AccessSize;
	uint64_t Address;
} ACPI_GAS;

typedef struct {
	ACPI_TABLE_HEADER Header;
	uint32_t FirmwareCtrl;
	uint32_t Dsdt;

	uint8_t _reserved;

	uint8_t PreferredPowerManagementProfile;
	uint16_t SCI_Interrupt;
	uint32_t SMI_CommandPort;
	uint8_t AcpiEnable;
	uint8_t AcpiDisable;
	uint8_t S4BIOS_REQ;
	uint8_t PSTATE_Control;
	uint32_t PM1aEventBlock;
	uint32_t PM1bEventBlock;
	uint32_t PM1aControlBlock;
	uint32_t PM1bControlBlock;
	uint32_t PM2ControlBlock;
	uint32_t PMTimerBlock;
	uint32_t GPE0Block;
	uint32_t GPE1Block;
	uint8_t PM1EventLength;
	uint8_t PM1ControlLength;
	uint8_t PM2ControlLength;
	uint8_t PMTimerLength;
	uint8_t GPE0Length;
	uint8_t GPE1Length;
	uint8_t GPE1Base;
	uint8_t CStateControl;
	uint16_t WorstC2Latency;
	uint16_t WorstC3Latency;
	uint16_t FlushSize;
	uint16_t FlushStride;
	uint8_t DutyOffset;
	uint8_t DutyWidth;
	uint8_t DayAlarm;
	uint8_t MonthAlarm;
	uint8_t Century;

	uint16_t BootArchitectureFlags;

	uint8_t Reserved2;
	uint32_t Flags;

	ACPI_GAS ResetReg;

	uint8_t ResetValue;
	uint8_t Reserved3[3];

	uint64_t X_FirmwareControl;
	uint64_t X_Dsdt;

	ACPI_GAS X_PM1aEventBlock;
	ACPI_GAS X_PM1bEventBlock;
	ACPI_GAS X_PM1aControlBlock;
	ACPI_GAS X_PM1bControlBlock;
	ACPI_GAS X_PM2ControlBlock;
	ACPI_GAS X_PMTimerBlock;
	ACPI_GAS X_GPE0Block;
	ACPI_GAS X_GPE1Block;
} __attribute__((packed)) ACPI_FADT;

#endif
