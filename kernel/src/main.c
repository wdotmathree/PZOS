#include <incbin.h>
#include <limine.h>

#include <stdbool.h>
#include <stdio.h>

#include <kernel/intrin.h>
#include <kernel/isr.h>
#include <kernel/panic.h>
#include <kernel/serial.h>
#include <kernel/tty.h>

__attribute__((used, section(".limine_requests_start"))) //
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests"))) //
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests"))) //
static volatile struct limine_framebuffer_request framebuffer_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST,
	.revision = 0
};

__attribute__((used, section(".limine_requests_end"))) //
static volatile LIMINE_REQUESTS_END_MARKER;

__attribute__((noreturn)) void kmain(void);

void kinit(void) {
	if (!LIMINE_BASE_REVISION_SUPPORTED)
		halt();
	if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1)
		halt();
	tty_init(framebuffer_request.response->framebuffers[0]);

	// Initialize IDT
	extern void isr_stub_0;
	for (int i = 0; i < IDT_SIZE; i++) {
		register_isr(i, &isr_stub_0 + (16 * i), 0);
	}
	idt_load();
	asm("sti");

	// Initialize serial port
	if (serial_init()) {
		tty_puts("Serial port initialization failed!\n");
		halt();
	}

	kmain();
	panic("kmain returned unexpectedly");
}

void kmain(void) {
	tty_puts("PZOS booted successfully!\n");

	int a = 1;
	int b = 1;
	while (true) {
		int c = a + b;
		printf("%d\n", a);
		a = b;
		b = c;
		for (int i = 0; i < 500000000; i++) {
			asm("nop");
		}
	}

	while (true)
		halt();
	panic(NULL);
}
