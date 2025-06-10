#include <kernel/serial.h>

#include <stdbool.h>

#include <kernel/intrin.h>

#define COM1 (0x3f8)

static bool ready = false;

int serial_init(void) {
	outb(COM1 + 1, 0x00); // Disable interrupts
	outb(COM1 + 3, 0x80); // Divide baud rate
	outb(COM1 + 0, 0x03); // ... to 38400 baud (lo byte)
	outb(COM1 + 1, 0x00); // ... (hi byte)
	outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
	outb(COM1 + 2, 0x07); // Enable and clear FIFOs, 1 byte threshold
	outb(COM1 + 4, 0x13); // Enable RTS/DTR, loopback for testing

	outb(COM1, 0xdb);
	if (inb(COM1) != 0xdb)
		return -1;

	outb(COM1 + 4, 0x03); // Disable loopback
	ready = true;

	return 0;
}

char serial_read(void) {
	if (!ready)
		return -1;

	while ((inb(COM1 + 5) & 0x01) == 0);
	return inb(COM1);
}

void serial_write(char c) {
	if (!ready)
		return;

	while ((inb(COM1 + 5) & 0x20) == 0);
	outb(COM1, c);
}
