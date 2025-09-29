#include <kernel/pci.h>

#include <x86_64/intrin.h>

#include <kernel/kmalloc.h>
#include <kernel/log.h>
#include <kernel/panic.h>

#include <string.h>

#define CONFIG_ADDR(bus, dev, func) ((mmio_bases[bus]) + (((dev) << 15) | ((func) << 12)))

extern uintptr_t hhdm_off;

static uint16_t num_segment_groups = 0;
static uintptr_t mmio_bases[65536];

static pci_bus_t *pci_tree = NULL;
static pci_dev_t *pci_devices = NULL;

static uint32_t pci_cfg_read_mmio(int bus, int dev, int func, size_t offset) {
	uintptr_t addr = CONFIG_ADDR(bus, dev, func) + offset;
	return *(volatile uint32_t *)(addr);
}

static void pci_cfg_write_mmio(int bus, int dev, int func, size_t offset, uint64_t data, size_t size) {
	uintptr_t addr = CONFIG_ADDR(bus, dev, func) + offset;
	if (size == 1)
		*(volatile uint8_t *)(addr) = data;
	else if (size == 2)
		*(volatile uint16_t *)(addr) = data;
	else if (size == 4) [[likely]]
		*(volatile uint32_t *)(addr) = data;
	else
		panic("Unsupported PCI config write size", "a");
}

/// TODO: Implement PIO access method

uint32_t (*pci_cfg_read)(int bus, int dev, int func, size_t offset) = pci_cfg_read_mmio;
void (*pci_cfg_write)(int bus, int dev, int func, size_t offset, uint64_t data, size_t size) = pci_cfg_write_mmio;

static pci_dev_t *pci_add_device(int seg_group, pci_bus_t *bus, int dev, int func) {
	pci_dev_t *new_dev = kmalloc(sizeof(pci_dev_t));
	*new_dev = (pci_dev_t){
		.next = pci_devices,
		.sibling = bus->devices,
		.bus = bus->bus,
		.device = dev,
		.function = func,
	};
	pci_devices = new_dev;
	bus->devices = new_dev;

	return new_dev;
}

static void pci_enumerate_bus(int seg_group, int bus, uintptr_t base_addr, pci_bus_t *bus_ptr);
static bool pci_check_func(int seg_group, int bus, int dev, int func, uintptr_t mmio_base, pci_bus_t *bus_ptr) {
	void *cfg_addr = (void *)(mmio_base + ((dev << 15) | (func << 12)));

	uint16_t vendor_id = *(uint16_t *)(cfg_addr + 0x00);
	if (vendor_id == 0xffff)
		return false;

	uint16_t device_id = *(uint16_t *)(cfg_addr + 0x02);
	uint8_t header_type = *(uint8_t *)(cfg_addr + 0x0e);
	uint32_t class = *(uint32_t *)(cfg_addr + 0x08);
	LOG("PCI", "%04x:%02x:%02x.%d: [%04x:%04x] type %02x class 0x%06x", seg_group, bus, dev, func, vendor_id, device_id, header_type & 0x7f, class >> 8);

	pci_dev_t *new_dev = pci_add_device(seg_group, bus_ptr, dev, func);

	if ((header_type & 0x7f) == 1) {
		// PCI-to-PCI bridge
		uint8_t secondary_bus = *(uint8_t *)(cfg_addr + 0x19);
		uintptr_t sec_mmio_base = mmio_bases[secondary_bus];
		if (sec_mmio_base == 0) {
			LOG("PCI", "Warning: Secondary bus %u has no MMIO base, skipping", secondary_bus);
		} else {
			LOG("PCI", "Found PCI-to-PCI bridge to bus %u", secondary_bus);

			pci_bus_t *new_bus = kmalloc(sizeof(pci_bus_t));
			*new_bus = (pci_bus_t){
				.parent = bus_ptr,
				.children = NULL,
				.next = bus_ptr->children,
				.bridge = new_dev,
				.devices = NULL,
				.bus = secondary_bus,
			};
			bus_ptr->children = new_bus;

			pci_enumerate_bus(seg_group, secondary_bus, sec_mmio_base, new_bus);
		}
	}

	// Check for multifunction device
	if (func == 0 && header_type & 0x80) {
		// Enumerate other functions
		for (int fun = 1; fun < 8; fun++) {
			if (!pci_check_func(seg_group, bus, dev, fun, mmio_base, bus_ptr))
				continue;
		}
	}
	return true;
}

static void pci_enumerate_bus(int seg_group, int bus, uintptr_t mmio_base, pci_bus_t *bus_ptr) {
	for (int dev = 0; dev < 32; dev++) {
		if (!pci_check_func(seg_group, bus, dev, 0, mmio_base, bus_ptr))
			continue;
	}
}

void pci_init(void) {
	ACPI_MCFG *table = acpi_get_table(ACPI_SIG("MCFG"));

	pci_tree = kmalloc(sizeof(pci_bus_t));
	*pci_tree = (pci_bus_t){
		.next = NULL,
		.parent = NULL,
		.children = NULL,
		.devices = NULL,
		.bus = 0,
	};

	memset(mmio_bases, 0, sizeof(mmio_bases));
	if (table == NULL) {
		LOG("PCI", "No MCFG table found, skipping PCI initialization");
		/// TODO: Implement legacy PIO access method
	} else {
		num_segment_groups = (table->Header.Length - sizeof(ACPI_TABLE_HEADER) - 8) / sizeof(ACPI_MCFG_ENTRY);

		LOG("PCI", "Found %u segment groups", num_segment_groups);
		for (uint16_t seg_group = 0; seg_group < num_segment_groups; seg_group++) {
			ACPI_MCFG_ENTRY *entry = &table->entries[seg_group];
			LOG("PCI", "Segment group %u: base address %p, start bus %u, end bus %u",
				seg_group, entry->base_address, entry->start_bus_number, entry->end_bus_number);

			for (int bus = entry->start_bus_number; bus <= entry->end_bus_number; bus++)
				mmio_bases[bus] = entry->base_address + ((uintptr_t)bus << 20) + hhdm_off;

			// Begin recursive enumeration
			pci_enumerate_bus(seg_group, entry->start_bus_number, mmio_bases[entry->start_bus_number], pci_tree);
		}
	}

	LOG("PCI", "PCI initialization complete");
}
