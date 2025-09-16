#include <kernel/pci.h>

#include <x86_64/intrin.h>

#include <kernel/kmalloc.h>
#include <kernel/log.h>
#include <kernel/panic.h>
#include <kernel/vmem.h>

#include <string.h>

#define CONFIG_ADDR(bus, dev, func) ((mmio_bases[bus]) + (((dev) << 15) | ((func) << 12)))

extern uintptr_t hhdm_off;

static uint16_t num_segment_groups = 0;
static uintptr_t mmio_bases[256];

static uint64_t pci_cfg_read_mmio(int bus, int dev, int func, size_t size) {
	uintptr_t addr = CONFIG_ADDR(bus, dev, func);
	uint64_t data = 0;
	if (size == 1)
		data = *(volatile uint8_t *)(addr);
	else if (size == 2)
		data = *(volatile uint16_t *)(addr);
	else if (size == 4)
		data = *(volatile uint32_t *)(addr);
	else if (size == 8)
		data = *(volatile uint64_t *)(addr);
	else
		data = -1;
	return data;
}

static void pci_cfg_write_mmio(int bus, int dev, int func, uint64_t data, size_t size) {
	uintptr_t addr = CONFIG_ADDR(bus, dev, func);
	if (size == 1)
		*(volatile uint8_t *)(addr) = data;
	else if (size == 2)
		*(volatile uint16_t *)(addr) = data;
	else if (size == 4)
		*(volatile uint32_t *)(addr) = data;
	else if (size == 8)
		*(volatile uint64_t *)(addr) = data;
	else
		panic("Unsupported PCI config write size", "a");
}

uint64_t (*pci_cfg_read)(int bus, int dev, int func, size_t size) = pci_cfg_read_mmio;

static void *pci_check_func(int seg_group, int bus, int dev, int func, uintptr_t mmio_base, void *prev_mapping) {
	void *cfg_addr = vmalloc_at(prev_mapping, (void *)VMEM_MMIO_END, 1, VMA_READ | VMA_WRITE);
	prev_mapping = cfg_addr;
	map_page(cfg_addr, (void *)(mmio_base + (dev << 15) + (func << 12)), PAGE_PRESENT | PAGE_RW | PAGE_NX);

	uint16_t vendor_id = *(uint16_t *)(cfg_addr + 0x00);
	if (vendor_id == 0xffff) {
		vfree(cfg_addr, 1);
		unmap_page(cfg_addr);

		return NULL;
	}
	uint16_t device_id = *(uint16_t *)(cfg_addr + 0x02);
	uint8_t header_type = *(uint8_t *)(cfg_addr + 0x0e);
	uint32_t class = *(uint32_t *)(cfg_addr + 0x08);
	LOG("PCI", "%04x:%02x:%02x.%d: [%04x:%04x] type %02x class 0x%08x", seg_group, bus, dev, func, vendor_id, device_id, header_type & 0x7f, class);

	// Check for multifunction device
	if (func == 0 && header_type & 0x80) {
		// Enumerate other functions
		for (int fun = 1; fun < 8; fun++) {
			void *tmp = pci_check_func(seg_group, bus, dev, fun, mmio_base, prev_mapping);
			if (tmp == NULL)
				continue;
			else
				prev_mapping = tmp;
		}
	}
}

static void pci_enumerate_bus(int seg_group, int bus, uintptr_t base_addr) {
	LOG("PCI", "Enumerating bus %02x", bus);
	uintptr_t mmio_base = base_addr + (bus << 20);

	void *prev_mapping = (void *)VMEM_MMIO_BASE;
	for (int dev = 0; dev < 32; dev++) {
		void *tmp = pci_check_func(seg_group, bus, dev, 0, mmio_base, prev_mapping);

		if (tmp == NULL)
			continue;
		else
			prev_mapping = tmp;
	}
}

void pci_init(void) {
	ACPI_MCFG *table = acpi_get_table(ACPI_SIG("MCFG"));

	memset(mmio_bases, 0, sizeof(mmio_bases));
	if (table == NULL) {
		LOG("PCI", "No MCFG table found, trying legacy initialization");
		/// TODO: Implement legacy initialization
	} else {
		num_segment_groups = (table->Header.Length - sizeof(ACPI_TABLE_HEADER) - 8) / sizeof(ACPI_MCFG_ENTRY);

		LOG("PCI", "Found %u segment groups", num_segment_groups);
		for (uint16_t seg_group = 0; seg_group < num_segment_groups; seg_group++) {
			ACPI_MCFG_ENTRY *entry = &table->entries[seg_group];
			LOG("PCI", "Segment group %u: base address %p, start bus %u, end bus %u",
				seg_group, mmio_bases[seg_group], entry->start_bus_number, entry->end_bus_number);

			// Begin recursive enumeration
			pci_enumerate_bus(seg_group, entry->start_bus_number, entry->base_address);
		}
	}

	LOG("PCI", "PCI initialization complete");
}
