#include <kernel/isr.h>

#include <stddef.h>
#include <stdio.h>

#include <kernel/defines.h>
#include <kernel/panic.h>

struct isr_frame_t *default_isr(struct isr_frame_t *const);

static struct idt_entry idt[IDT_SIZE];
struct isr_frame_t *(*isr_vectors[IDT_SIZE])(struct isr_frame_t *const);

void isr_init(void) {
	extern void isr_stub_0;
	for (int i = 0; i < IDT_SIZE; i++) {
		uintptr_t stub_addr = (uintptr_t)&isr_stub_0 + i * 16;
		struct idt_entry *entry = &idt[i];
		entry->isr_low = stub_addr & 0xffff;
		entry->kernel_cs = KERNEL_CS;
		entry->ist = 0;
		entry->attributes = 0x8e;
		entry->isr_mid = (stub_addr >> 16) & 0xffff;
		entry->isr_high = (stub_addr >> 32) & 0xffffffff;

		isr_vectors[i] = default_isr;
	}

	struct {
		uint16_t limit;
		uintptr_t base;
	} __attribute__((packed)) idtr = {
		.limit = sizeof(idt) - 1,
		.base = (uintptr_t)idt
	};
	asm volatile("lidt %0" : : "m"(idtr));
}

void register_isr(uint8_t num, void *handler, uint8_t dpl) {
	idt[num].attributes = (dpl << 5) | 0x8e; // Set DPL and present bit
	isr_vectors[num] = handler;
}

struct isr_frame_t *default_isr(struct isr_frame_t *const frame) {
	switch (frame->irq_num) {
	case 0x81: // Custom panic IRQ
		if (frame->isr_cs != KERNEL_CS) {
			// Misbehaving userland code
			// kill();
			break;
		}
		_panic(*(const char **)frame->isr_rsp, frame);
	default:
		printf("\nIRQ %u triggered with error code %u\n", frame->irq_num, frame->error_code);
		_panic("Unhandled IRQ", frame);
	}

	return frame;
}
