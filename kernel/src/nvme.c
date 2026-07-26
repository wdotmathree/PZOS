#include <kernel/nvme.h>

#include <kernel/kmalloc.h>
#include <kernel/log.h>
#include <kernel/paging.h>
#include <kernel/pci.h>

typedef struct nvme_mmio_t {
	uint64_t cap;
	uint32_t vs;
	uint32_t intms;
	uint32_t intmc;
	uint32_t cc;
	uint8_t _res_0[0x1c - 0x18];
	uint32_t csts;
	uint32_t nssr;
	uint32_t aqa;
	uint64_t asq;
	uint64_t acq;
	uint32_t cmbloc;
	uint32_t cmbsz;
	uint32_t bpinfo;
	uint32_t bprsel;
	uint64_t bpmbl;
	uint64_t cmbmsc;
	uint32_t cmbsts;
	uint32_t cmbebs;
	uint32_t cmbswtp;
	uint32_t nssd;
	uint32_t nrto;
	uint8_t _res_1[0xe00 - 0x6c];
	uint32_t pmrcap;
	uint32_t pmrctl;
	uint32_t pmrsts;
	uint32_t pmrebs;
	uint32_t pmrswtp;
	uint32_t pmrmscl;
	uint32_t pmrmscu;
} __attribute__((packed)) nvme_mmio_t;

typedef struct nvme_queue_t {
	uint16_t id;
	uint16_t sq_size;
	uint16_t cq_size;
	volatile uint32_t *doorbell_addr;
	void *sq_buf;
	void *cq_buf;
	uint16_t sq_head;
	uint16_t cq_head;
} nvme_queue_t;

typedef struct nvme_ctrl_t {
	volatile nvme_mmio_t *mmio;
	uint16_t dstrd; // Doorbell stride measured in DWORDS (not bytes)
	uint8_t timeout; // 500 millisecond units
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
	volatile nvme_mmio_t *mmio = (void *)__va(((uint64_t)header->bar1 << 32) | (header->bar0 & ~0x0f));
	map_page((void *)mmio, __pa(mmio), PAGE_RW | PAGE_NX | PAGE_TYPE(PAT_UC));
	LOG("NVMe", "Found NVMe controller at %02x:%02x.%x, MMIO base: %p", dev->bus, dev->device, dev->function, __pa(mmio));

	// Setup controller struct
	nvme_ctrl_t *ctrl = kmalloc(sizeof(nvme_ctrl_t));
	ctrl->mmio = mmio;
	ctrl->dstrd = 1 << ((mmio->cap >> 32) & 0x0f); // We don't need the 2 + since a dword is already 4 bytes
	ctrl->timeout = (mmio->cap >> 24) & 0xff;
	// Reset the controller
	mmio->cc &= ~0x1; // CC.EN = 0
	uint64_t start = get_utime();
	uint64_t timeout = start + ctrl->timeout * 500000;
	while (mmio->csts & 0x01 && get_utime() < timeout); // Wait for CSTS.RDY = 0
	if (mmio->csts & 0x01) {
		LOG("NVMe", "%02x:%02x.%x: timed out while waiting for controller reset", dev->bus, dev->device, dev->function);
		return false;
	}

	// Set up admin queue
	ctrl->admin_q.id = 0;
	ctrl->admin_q.sq_size = 64;
	ctrl->admin_q.cq_size = 64;
	ctrl->admin_q.sq_buf = __va(alloc_page());
	ctrl->admin_q.cq_buf = __va(alloc_page());
	ctrl->admin_q.doorbell_addr = (void *)mmio + 0x1000;
	ctrl->admin_q.sq_head = 0;
	ctrl->admin_q.cq_head = 0;
	// Write attributes to controller
	mmio->aqa = ((ctrl->admin_q.cq_size - 1) << 16) | (ctrl->admin_q.sq_size - 1);
	mmio->asq = __pa(ctrl->admin_q.sq_buf);
	mmio->acq = __pa(ctrl->admin_q.cq_buf);

	// Configure controller command sets
	// Other I/O command sets are not yet implemented
	if (mmio->cap & (1ULL << 44)) { // CAP.CSS.NOIOCSS
		// No I/O command set supported, abort
		return false;
	} else if (mmio->cap & (1ULL << 43)) { // CAP.CSS.IOCSS
		// I/O command set supported, configure CC.CSS
		mmio->cc = (mmio->cc & 0xffffff8f) | (0b110 << 4);
	} else if (mmio->cap & (1ULL << 37)) { // CAP.CSS.NCSS
		// NVMe command set supported, configure CC.CSS
		mmio->cc = (mmio->cc & 0xffffff8f) | (0b000 << 4);
	} else {
		LOG("NVMe", "%02x:%02x.%x: no supported command set, aborting", dev->bus, dev->device, dev->function);
		return false;
	}
	// Set arbitration and page size
	mmio->cc = (mmio->cc & 0xffffc7ff) | (0b000 << 11); // CC.AMS = Round Robin
	mmio->cc = (mmio->cc & 0xfffff87f) | (0 << 7); // CC.MPS = 4KiB pages

	// Enable the controller
	mmio->cc |= 0x1; // CC.EN = 1
	start = get_utime();
	timeout = start + ctrl->timeout * 500000;
	while (!(mmio->csts & 0b11) && get_utime() < timeout); // Wait for CSTS.RDY = 1 or CSTS.CFS = 1
	if (!(mmio->csts & 0b11)) {
		LOG("NVMe", "%02x:%02x.%x: timed out while waiting for controller enable", dev->bus, dev->device, dev->function);
		return false;
	}
	if (mmio->csts & 0b10) { // CSTS.CFS = 1
		LOG("NVMe", "%02x:%02x.%x: failed to enable NVMe controller", dev->bus, dev->device, dev->function);
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
