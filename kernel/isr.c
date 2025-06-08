#include <kernel/isr.h>

#include <stdio.h>

#include <kernel/panic.h>

struct idt_entry idt[IDT_SIZE];

void idt_load(void) {
	struct {
		uint16_t limit;
		uintptr_t base;
	} __attribute__((packed)) idt_ptr = {
		.limit = sizeof(idt) - 1,
		.base = (uintptr_t)idt
	};

	asm volatile("lidt %0" : : "m"(idt_ptr));
}

void register_isr(uint8_t num, void *handler, uint8_t dpl) {
	struct idt_entry *entry = &idt[num];
	entry->isr_low = (uintptr_t)handler & 0xffff;
	entry->kernel_cs = 5 << 3;
	entry->ist = 0;
	entry->attributes = 0x8e | (dpl << 5);
	entry->isr_mid = ((uintptr_t)handler >> 16) & 0xffff;
	entry->isr_high = ((uintptr_t)handler >> 32) & 0xffffffff;
}

struct isr_frame_t *isr_dispatcher(struct isr_frame_t *const frame) {
	printf("ISR %u triggered with error code %u\n", frame->interrupt_number, frame->error_code);

	ipanic("Unhandled ISR", frame);

	return frame;
}
