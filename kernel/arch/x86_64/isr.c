#include <kernel/isr.h>

#include <stddef.h>
#include <stdio.h>

#include <kernel/defines.h>
#include <kernel/panic.h>

struct isr_frame_t *default_isr(struct isr_frame_t *const);

static struct gdt_entry gdt[5] = {
	{0}, // Null descriptor
	{0xffff, 0, 0, 0b1010111110011010, 0}, // Kernel code
	{0xffff, 0, 0, 0b1010111110010010, 0}, // Kernel data
	{0xffff, 0, 0, 0b1010111111111010, 0}, // User code
	{0xffff, 0, 0, 0b1010111111110010, 0}, // User data
};

static struct idt_entry idt[IDT_SIZE];
struct isr_frame_t *(*isr_vectors[IDT_SIZE])(struct isr_frame_t *const);

void isr_init(void) {
	// First deal with GDT stuff
	struct {
		uint16_t limit;
		uintptr_t base;
	} __attribute__((packed)) gdtr = {
		.limit = sizeof(gdt) - 1,
		.base = (uintptr_t)gdt
	};
	asm volatile("push 1 << 3\n"
				 "lea rax, [label]\n"
				 "push rax\n"
				 "lgdt %0\n"
				 "retfq\n"
				 "label:\n"
				 "mov eax, 2 << 3\n"
				 "mov ds, ax\n"
				 "mov es, ax\n"
				 "mov fs, ax\n"
				 "mov gs, ax\n"
				 "mov ss, ax"
				 : : "m"(gdtr) : "rax", "memory");

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
	asm volatile("lidt %0" : : "m"(idtr) : "memory");
}

void register_isr(uint8_t num, void *handler, uint8_t dpl) {
	idt[num].attributes = (dpl << 5) | 0x8e; // Set DPL and present bit
	isr_vectors[num] = handler;
}

struct isr_frame_t *default_isr(struct isr_frame_t *const frame) {
	switch (frame->irq_num) {
	default:
		printf("\nIRQ %u triggered with error code %u\n", frame->irq_num, frame->error_code);
		_panic(frame, "Unhandled IRQ");
	}

	return frame;
}
