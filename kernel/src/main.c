#define LIMINE_API_REVISION 3

#include <limine.h>

#include <stdbool.h>
#include <stdio.h>

#include <kernel/intrin.h>
#include <kernel/mman.h>
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
	.revision = 0,
};

__attribute__((used, section(".limine_requests"))) //
static volatile struct limine_memmap_request memory_map_request = {
	.id = LIMINE_MEMMAP_REQUEST,
	.revision = 0,
};

__attribute__((used, section(".limine_requests"))) //
static volatile struct limine_hhdm_request hhdm_request = {
	.id = LIMINE_HHDM_REQUEST,
	.revision = 0,
};

extern void _binary_size;

__attribute__((used, section(".limine_requests_end"))) //
static volatile LIMINE_REQUESTS_END_MARKER;

__attribute__((noreturn)) void kmain(void);

__attribute__((naked, noreturn)) void kinit(void) {
	// Pop the (bogus) return address to get the stack base
	// Save it in rbp (will be used in mman_init)
	asm volatile("add rsp, 8\n"
				 "mov rbp, rsp");

	asm("cli");
	if (!LIMINE_BASE_REVISION_SUPPORTED)
		halt();
	if (hhdm_request.response == NULL)
		halt();
	if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
		// Try printing error to VGA text mode if available
		uint16_t *vga = (uint16_t *)(hhdm_request.response->offset + 0xb8000);
		const char *s = "Could not get framebuffer! Stopping.";
		char c = s[0];
		for (int i = 0; i < 80 * 25; i++) {
			vga[i] = (0x0f << 8) | c;
			if (c != '\0')
				c = s[i + 1];
		}
		halt();
	}

	// Enable SSE
	asm volatile("mov rax, cr0\n"
				 "and rax, -5\n" // -5 = 0xfb
				 "or rax, 0x2\n"
				 "mov cr0, rax\n"
				 "mov rax, cr4\n"
				 "or rax, 0x00040600\n"
				 "mov cr4, rax"
				 : : : "rax");

	// Initialize interrupt handlers
	isr_init();

	tty_init(framebuffer_request.response->framebuffers[0]);
	tty_puts("PZOS kernel initializing...\n");

	// Initialize serial port
	if (serial_init())
		tty_puts("No serial port found, continuing without it.\n");
	else
		tty_puts("Serial port initialized successfully.\n");

	// Initialize memory management
	if (memory_map_request.response == NULL)
		panic("Memory map request failed");

	extern uint8_t *tty_buf;
	mman_init(memory_map_request.response, &tty_buf, hhdm_request.response->offset);

	kmain();
}

__attribute__((noreturn)) void kmain(void) {
	tty_puts("\nPZOS booted successfully!\n");

	while (true)
		halt();
	panic(NULL);
}
