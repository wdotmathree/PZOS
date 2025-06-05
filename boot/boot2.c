#include <intrin.h>
#include <stddef.h>
#include <stdint.h>
#include <tty.h>
#include <vga.h>

void (*kernel_main)(void) = (void *)0xffffffff8100000;

struct GateDescriptor32 {
	uint16_t offset_lo;
	uint16_t selector;
	uint8_t _zero;
	uint8_t type_attributes;
	uint16_t offset_hi;
} __attribute__((packed));

extern struct GateDescriptor32 idt32[];

struct InterruptFrame32 {
	uint32_t eflags;
	uint32_t cs;
	uint32_t eip;
} __attribute__((packed));

extern const void idtr32;

char letters[] = "0123456789ABCDEF";

void exception_handler(int num, int error) {
	terminal_initialize();
	terminal_initialize();
	terminal_writestring("Exception: ");

	terminal_putchar(letters[(num >> 4) & 0xf]);
	terminal_putchar(letters[(num >> 0) & 0xf]);

	terminal_writestring("\nError code: ");
	terminal_putchar(letters[(error >> 12) & 0xf]);
	terminal_putchar(letters[(error >> 8) & 0xf]);
	terminal_putchar(letters[(error >> 4) & 0xf]);
	terminal_putchar(letters[(error >> 0) & 0xf]);

	terminal_writestring("\nEIP: ");
	terminal_putchar(letters[(error >> 28) & 0xf]);
	terminal_putchar(letters[(error >> 24) & 0xf]);
	terminal_putchar(letters[(error >> 20) & 0xf]);
	terminal_putchar(letters[(error >> 16) & 0xf]);
	terminal_putchar(letters[(error >> 12) & 0xf]);
	terminal_putchar(letters[(error >> 8) & 0xf]);
	terminal_putchar(letters[(error >> 4) & 0xf]);
	terminal_putchar(letters[(error >> 0) & 0xf]);

	while (1)
		asm("hlt");
}

__attribute__((interrupt)) void master_null(struct InterruptFrame32 *frame) {
	// Read ISR
	outb(0x20, 0x0b);
	// Only send EOI if there is an interrupt pending
	if (inb(0x20))
		outb(0x20, 0x20);
}

__attribute__((interrupt)) void slave_null(struct InterruptFrame32 *frame) {
	// Read ISR
	outb(0xa0, 0x0b);
	// Only send EOI if there is an interrupt pending
	if (inb(0xa0)) {
		// Need to send EOI to both
		outb(0xa0, 0x20);
		outb(0x20, 0x20);
	}
}

void boot2(void) {
	outb(0x20, 0x11); // ICW1: Initialize with ICW4
	io_wait();
	outb(0xa0, 0x11); // ICW1: Initialize with ICW4
	io_wait();
	outb(0x21, 0x20); // ICW2: Remap master PIC to 0x20
	io_wait();
	outb(0xa1, 0x28); // ICW2: Remap slave PIC to 0x28
	io_wait();
	outb(0x21, 4); // ICW3: Cascade on IRQ2
	io_wait();
	outb(0xa1, 2); // ICW3: Cascade on IRQ2
	io_wait();
	outb(0x21, 0x01); // ICW4: 8086 mode
	io_wait();
	outb(0xa1, 0x01); // ICW4: 8086 mode
	io_wait();

	// Handle superious interrupts
	idt32[0x20 + 7] = (struct GateDescriptor32){
		.offset_lo = (uint16_t)((uintptr_t)master_null & 0xffff),
		.selector = 0x08,
		._zero = 0,
		.type_attributes = 0x8e, // Present, ring 0, interrupt gate
		.offset_hi = (uint16_t)(((uintptr_t)master_null >> 16) & 0xffff),
	};
	idt32[0x20 + 15] = (struct GateDescriptor32){
		.offset_lo = (uint16_t)((uintptr_t)slave_null & 0xffff),
		.selector = 0x08,
		._zero = 0,
		.type_attributes = 0x8e, // Present, ring 0, interrupt gate
		.offset_hi = (uint16_t)(((uintptr_t)slave_null >> 16) & 0xffff),
	};

	asm("lidt [%0]" : : "r"(&idtr32));

	// Unmask needed interrupts
	outb(0x21, 0b11111011); // Cascade
	outb(0xa1, 0b11111111); // None

	// Enable interrupts
	asm("sti");

	terminal_initialize();
	terminal_writestring("Hello, World!\n");

	kernel_main();

	while (1)
		asm("hlt");
}
