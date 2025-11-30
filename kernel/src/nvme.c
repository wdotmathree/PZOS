#include <kernel/nvme.h>

#include <kernel/kmalloc.h>
#include <kernel/log.h>
#include <kernel/paging.h>
#include <kernel/pci.h>

typedef struct nvme_queue_t {
	uint16_t id;
	uint16_t sq_size;
	uint16_t cq_size;
	uint64_t doorbell_addr;
	uint64_t *sq_buf;
	uint64_t *cq_buf;
	uint16_t sq_head;
	uint16_t cq_head;
} nvme_queue_t;

typedef struct nvme_ctrl_t {
	uintptr_t mmio_base;
	uint16_t dstrd;
	nvme_queue_t admin_q;
	nvme_queue_t io_q;
	struct nvme_ctrl_t *next;
} nvme_ctrl_t;

nvme_ctrl_t *nvme_ctrls = NULL;
size_t nvme_ctrl_count = 0;

bool nvme_attach(pci_dev_t *dev) {
	// Set up PCI device
	pci_dev_header_t *header = (pci_dev_header_t *)dev->header;
	dev->header->command |= PCI_CMD_BUS_MASTER | PCI_CMD_MEM_SPACE;
	dev->header->command &= ~PCI_CMD_INTERRUPT_DISABLE;
	// Map BAR0
	uintptr_t mmio_base = (((uint64_t)header->bar1 << 32) | (header->bar0 & ~0x0f)) + hhdm_off;
	map_page((void *)mmio_base, (void *)(mmio_base - hhdm_off), PAGE_RW | PAGE_NX | PAGE_TYPE(PAT_UC));
	LOG("NVMe", "Found NVMe controller at %02x:%02x.%x, MMIO base: %p", dev->bus, dev->device, dev->function, mmio_base - hhdm_off);

	// Initialize NVMe controller
	nvme_ctrl_t *ctrl = kmalloc(sizeof(nvme_ctrl_t));
	ctrl->mmio_base = mmio_base;
	ctrl->dstrd = 1 << ((*(volatile uint64_t *)(mmio_base) >> 32) & 0x0f);
	// Reset the controller
	*(volatile uint32_t *)(mmio_base + 0x14) &= ~0x1; // CC.EN = 0
	while (*(volatile uint32_t *)(mmio_base + 0x1c) & 0x1); // Wait for CSTS.RDY = 0

	// Set up admin queue
	ctrl->admin_q.id = 0;
	ctrl->admin_q.sq_size = 64;
	ctrl->admin_q.cq_size = 64;
	ctrl->admin_q.sq_buf = alloc_page();
	ctrl->admin_q.cq_buf = alloc_page();
	ctrl->admin_q.doorbell_addr = mmio_base + 0x1000;
	ctrl->admin_q.sq_head = 0;
	ctrl->admin_q.cq_head = 0;
	// Write attributes to controller
	*(volatile uint32_t *)(mmio_base + 0x24) = ((ctrl->admin_q.cq_size - 1) << 16) | (ctrl->admin_q.sq_size - 1); // AQA
	*(volatile uint64_t *)(mmio_base + 0x28) = (uintptr_t)ctrl->admin_q.cq_buf; // ASQ
	*(volatile uint64_t *)(mmio_base + 0x30) = (uintptr_t)ctrl->admin_q.sq_buf; // ACQ

	// Configure controller command sets
	// Other I/O command sets are not yet implemented
	if (*(volatile uint64_t *)mmio_base & (1ULL << 44)) { // CAP.CSS.NOIOCSS
		// No I/O command set supported, abort
		return false;
	} else if (*(volatile uint64_t *)mmio_base & (1ULL << 43)) { // CAP.CSS.IOCSS
		// I/O command set supported, configure CC.CSS
		*(volatile uint32_t *)(mmio_base + 0x14) = (*(volatile uint32_t *)(mmio_base + 0x14) & 0xffffff8f) | (0b110 << 4);
	} else if (*(volatile uint64_t *)mmio_base & (1ULL << 42)) { // CAP.CSS.NCSS
		// NVMe command set supported, configure CC.CSS
		*(volatile uint32_t *)(mmio_base + 0x14) = (*(volatile uint32_t *)(mmio_base + 0x14) & 0xffffff8f) | (0b000 << 4);
	} else {
		// No supported command set, abort
		return false;
	}
	// Set arbitration and page size
	*(volatile uint32_t *)(mmio_base + 0x14) = (*(volatile uint32_t *)(mmio_base + 0x14) & 0xffffc7ff) | (0b000 << 11); // CC.AMS = Round Robin
	*(volatile uint32_t *)(mmio_base + 0x14) = (*(volatile uint32_t *)(mmio_base + 0x14) & 0xfffff87f) | (0 << 7); // CC.MPS = 4KiB pages

	// Enable the controller
	*(volatile uint32_t *)(mmio_base + 0x14) |= 0x1; // CC.EN = 1
	while (!(*(volatile uint32_t *)(mmio_base + 0x1c) & 0b11)); // Wait for CSTS.RDY = 1 or CSTS.CFS = 1
	if (*(volatile uint32_t *)(mmio_base + 0x1c) & 0b10) { // CSTS.CFS = 1
		LOG("NVME", "Failed to enable NVMe controller at %02x:%02x.%x", dev->bus, dev->device, dev->function);
		return false;
	}

	// Add controller to list
	ctrl->next = nvme_ctrls;
	nvme_ctrls = ctrl;
	nvme_ctrl_count++;

	return true;
}

void nvme_init(void) {
	pci_driver_t nvme_driver = {
		.device_id = PCI_ID_ANY,
		.vendor_id = PCI_ID_ANY,
		.class_code = 0x010800,
		.class_mask = 0xffff00,
		.attach = nvme_attach
	};
	pci_register_driver(&nvme_driver);
}
