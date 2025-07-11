#pragma once
#ifndef ARCH_X86_64_PIC_H
#define ARCH_X86_64_PIC_H

#include <x86_64/intrin.h>

static void PIC_init(void) {
	outb(0x20, 0x11); // Init with ICW4
	io_wait();
	outb(0xa0, 0x11);
	io_wait();
	outb(0x21, 0x20); // Base at 0x20 (IRQ0)
	io_wait();
	outb(0xa1, 0x28); // cont...
	io_wait();
	outb(0x21, 4); // Secondary PIC is at IRQ2 (0000 0100)
	io_wait();
	outb(0xa1, 2); // Cascade on IRQ2
	io_wait();

	outb(0x21, 0x01); // Use 8086 mode (not 8080 mode)
	io_wait();
	outb(0xa1, 0x01);
	io_wait();

	// Mask everything
	outb(0x21, 0xff);
	outb(0xa1, 0xff);
}

static void PIC_mask(uint8_t irq) {
	if (irq < 8) {
		uint8_t mask = inb(0x21);
		mask |= (1 << irq);
		outb(0x21, mask);
	} else {
		uint8_t mask = inb(0xa1);
		mask |= (1 << (irq - 8));
		outb(0xa1, mask);
	}
}

static void PIC_unmask(uint8_t irq) {
	if (irq < 8) {
		uint8_t mask = inb(0x21);
		mask &= ~(1 << irq);
		outb(0x21, mask);
	} else {
		uint8_t mask = inb(0xa1);
		mask &= ~(1 << (irq - 8));
		outb(0xa1, mask);
	}
}

static void PIC_eoi(uint8_t irq) {
	if (irq < 8) {
		outb(0x20, 0x20);
	} else {
		outb(0xa0, 0x20);
		outb(0x20, 0x20);
	}
}

#endif
