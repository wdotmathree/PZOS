#include <x86_64/isr.h>

#include <stddef.h>
#include <stdio.h>

#include <x86_64/pic.h>

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

isr_handler_t isr_vectors[IDT_SIZE];

void isr_init(void) {
	// First deal with GDT stuff
	struct {
		uint16_t limit;
		uintptr_t base;
	} __attribute__((packed)) gdtr = {
		.limit = sizeof(gdt) - 1,
		.base = (uintptr_t)gdt
	};
	asm volatile("push 1 << 3\n\t"
				 "lea rax, [.Llabel]\n\t"
				 "push rax\n\t"
				 "lgdt %0\n\t"
				 "retfq\n\t"
				 ".Llabel:\n\t"
				 "mov eax, 2 << 3\n\t"
				 "mov ds, ax\n\t"
				 "mov es, ax\n\t"
				 "mov fs, ax\n\t"
				 "mov gs, ax\n\t"
				 "mov ss, ax"
				 : : "m"(gdtr) : "rax", "memory");

	// Populate our IDT
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

	// Initialize PICs
	PIC_init();
}

void register_isr(uint8_t num, isr_handler_t handler, uint8_t dpl) {
	idt[num].attributes = (dpl << 5) | 0x8e; // Set DPL and present bit
	isr_vectors[num] = handler;
}

void unregister_isr(uint8_t num) {
	idt[num].attributes = (0 << 5) | 0x8e; // Clear attributes to disable the ISR
	isr_vectors[num] = default_isr; // Reset to default handler
}

struct isr_frame_t *default_isr(struct isr_frame_t *const frame) {
	switch (frame->irq_num) {
	case 0x08: // Double fault
		// Too unsafe to call panic, just print state and halt
		printf("Double fault occurred! RIP: %p\n", frame->isr_rip);
		hcf();
	default:
		_panic(frame, "Unhandled IRQ %u, error code %u", frame->irq_num, frame->error_code);
	}

	return frame;
}
