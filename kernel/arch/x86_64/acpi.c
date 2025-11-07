#include <kernel/acpi.h>

#include <stdio.h>

#include <kernel/log.h>
#include <kernel/paging.h>

extern uintptr_t hhdm_off;

static uint8_t calculate_checksum(void *ptr, size_t size) {
	uint8_t sum = 0;
	for (size_t i = 0; i < size; i++) {
		sum += ((uint8_t *)ptr)[i];
	}
	return sum;
}

ACPI_TABLE_HEADER *rsdt = NULL;

void acpi_init(EFI_SYSTEM_TABLE *system_table) {
	ACPI_RSDP *rsdp = NULL;

	if (system_table == NULL) {
		uint64_t search_string = 0x2052545020445352; // "RSD PTR "

		uint16_t *ebda_seg = (uint16_t *)(hhdm_off + 0x040e); // EBDA segment address
		uint64_t *ptr = (uint64_t *)(hhdm_off + (*ebda_seg << 4)); // Beginning of EBDA
		for (size_t i = 0; i < 0x400 / 16; i++) {
			if (ptr[i] == search_string) {
				rsdp = (ACPI_RSDP *)(ptr + i);
				if (calculate_checksum(rsdp, sizeof(ACPI_RSDP)) == 0) {
					if (rsdp->Revision >= 2 && calculate_checksum(rsdp, sizeof(ACPI_XSDP)) != 0)
						rsdp = NULL;
					else
						break;
				} else {
					rsdp = NULL;
				}
			}
		}

		if (rsdp == NULL) {
			ptr = (uint64_t *)(hhdm_off + 0xe0000);
			for (size_t i = 0; i < 0x10000 / 16; i++) { // 0xe0000 to 0xfffff
				if (ptr[i] == search_string) {
					rsdp = (ACPI_RSDP *)(ptr + i);
					if (calculate_checksum(rsdp, sizeof(ACPI_RSDP)) == 0) {
						if (rsdp->Revision >= 2 && calculate_checksum(rsdp, sizeof(ACPI_XSDP)) != 0)
							rsdp = NULL;
						else
							break;
					} else {
						rsdp = NULL;
					}
				}
			}
		}

		if (rsdp != NULL)
			LOG("ACPI", "RSDP found at %p", rsdp);
	} else {
		map_page((void *)(hhdm_off + (uintptr_t)system_table), system_table, PAGE_NX | PAGE_TYPE(PAT_UC));
		system_table = (EFI_SYSTEM_TABLE *)(hhdm_off + (uintptr_t)system_table);

		// Search for ACPI tables in the EFI system table
		EFI_GUID guid = EFI_ACPI_20_TABLE_GUID;
		uint64_t *search_term = (uint64_t *)&guid;
		EFI_CONFIGURATION_TABLE *tables = (void *)(hhdm_off + (uintptr_t)system_table->ConfigurationTable);
		for (size_t i = 0; i < system_table->NumberOfTableEntries; i++) {
			if (((uint64_t *)&tables[i].VendorGuid)[0] == search_term[0] &&
				((uint64_t *)&tables[i].VendorGuid)[1] == search_term[1]) {
				rsdp = (ACPI_RSDP *)(hhdm_off + (uintptr_t)tables[i].VendorTable);
				map_page(rsdp, tables[i].VendorTable, PAGE_NX | PAGE_TYPE(PAT_UC));
				if (calculate_checksum(rsdp, sizeof(ACPI_RSDP)) == 0 &&
					calculate_checksum(rsdp, sizeof(ACPI_XSDP)) == 0)
					break;
				else
					rsdp = NULL;
			}
		}
	}

	if (rsdp == NULL) {
		LOG("ACPI", "No RSDP found. ACPI support is disabled.");
		return;
	}
	if (rsdp->Revision >= 2) {
		ACPI_XSDP *xsdp = (ACPI_XSDP *)rsdp;
		rsdt = (ACPI_TABLE_HEADER *)(hhdm_off + xsdp->XsdtAddress);
		map_page(rsdt, (void *)xsdp->XsdtAddress, PAGE_NX | PAGE_TYPE(PAT_UC));
	} else {
		LOG("ACPI", "ACPI revisions below 2.0 are not supported.");
		return;
	}

	// List ACPI tables
	ACPI_TABLE_HEADER **tables = (ACPI_TABLE_HEADER **)((uintptr_t)rsdt + sizeof(ACPI_TABLE_HEADER));
	uint32_t num_entries = (rsdt->Length - sizeof(ACPI_TABLE_HEADER)) / sizeof(uint64_t);
	for (uint32_t i = 0; i < num_entries; i++) {
		ACPI_TABLE_HEADER *table = (ACPI_TABLE_HEADER *)(hhdm_off + (uintptr_t)tables[i]);
		map_page(table, (void *)tables[i], PAGE_NX | PAGE_TYPE(PAT_UC));
		if (calculate_checksum(table, table->Length) == 0) {
			LOG("ACPI", "%.4s %p v%02hhu %-6.6s %-8.8s", table->Signature, table, table->Revision, table->OEMID, table->OEMTableID);
		} else {
			LOG("ACPI", "%.4s Checksum incorrect", table->Signature);
		}
	}
}

void *acpi_get_table(acpi_sig_t signature) {
	if (rsdt == NULL)
		return NULL;

	ACPI_TABLE_HEADER **tables = (ACPI_TABLE_HEADER **)((uintptr_t)rsdt + sizeof(ACPI_TABLE_HEADER));
	uint32_t num_entries = (rsdt->Length - sizeof(ACPI_TABLE_HEADER)) / sizeof(uint64_t);
	for (uint32_t i = 0; i < num_entries; i++) {
		ACPI_TABLE_HEADER *table = (ACPI_TABLE_HEADER *)(hhdm_off + (uintptr_t)tables[i]);
		if (*(uint32_t *)&table->Signature == signature) {
			if (calculate_checksum(table, table->Length) == 0) {
				return (void *)((uintptr_t)table);
			} else {
				LOG("ACPI", "Checksum mismatch for table with signature %c%c%c%c",
					(signature >> 24) & 0xFF, (signature >> 16) & 0xFF,
					(signature >> 8) & 0xFF, signature & 0xFF);
			}
		}
	}

	return NULL;
}
