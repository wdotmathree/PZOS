#include <kernel/pci.h>

#include <x86_64/intrin.h>

#include <kernel/kmalloc.h>
#include <kernel/log.h>

extern uintptr_t hhdm_off;

static uint16_t num_segment_groups = 0;
static uintptr_t *mmio_bases = NULL;

void pci_init(void) {
	ACPI_MCFG *table = acpi_get_table(ACPI_SIG("MCFG"));

	if (table == NULL) {
		LOG("PCI", "No MCFG table found, trying legacy initialization");
	} else {
		num_segment_groups = (table->Header.Length - sizeof(ACPI_TABLE_HEADER) - 8) / sizeof(ACPI_MCFG_ENTRY);
		mmio_bases = kmalloc(num_segment_groups * sizeof(uintptr_t));
		if (mmio_bases == NULL) {
			LOG("PCI", "kmalloc returned NULL, cannot initialize PCI");
			return;
		}

		LOG("PCI", "Found %u segment groups", num_segment_groups);
		for (uint16_t i = 0; i < num_segment_groups; i++) {
			ACPI_MCFG_ENTRY *entry = &table->entries[i];
			mmio_bases[i] = hhdm_off + entry->base_address;
			LOG("PCI", "Segment group %u: base address %p, start bus %u, end bus %u",
				i, mmio_bases[i], entry->start_bus_number, entry->end_bus_number);
		}
	}
}
