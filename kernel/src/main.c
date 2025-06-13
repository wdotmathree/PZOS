#define LIMINE_API_REVISION 3

#include <limine.h>

#include <stdbool.h>
#include <stdio.h>

#include <kernel/intrin.h>
#include <kernel/panic.h>
#include <kernel/pmman.h>
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

__attribute__((used, section(".limine_requests"))) //
static volatile struct limine_executable_address_request addr_request = {
	.id = LIMINE_EXECUTABLE_ADDRESS_REQUEST,
	.revision = 0,
};

extern void _binary_size;

__attribute__((used, section(".limine_requests_end"))) //
static volatile LIMINE_REQUESTS_END_MARKER;
__attribute__((noreturn)) void kmain(void);

void kinit(void) {
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
			if (c)
				c = s[i + 1];
		}
		halt();
	}
	tty_init(framebuffer_request.response->framebuffers[0]);
	tty_puts("PZOS kernel initializing...\n");

	// Initialize IDT
	extern void isr_stub_0;
	for (int i = 0; i < IDT_SIZE; i++) {
		register_isr(i, &isr_stub_0 + (16 * i), 0);
	}
	idt_load();
	asm("sti");

	// Initialize serial port
	if (serial_init())
		tty_puts("No serial port found, continuing without it.\n");
	else
		tty_puts("Serial port initialized successfully.\n");

	// Initialize memory management
	if (memory_map_request.response == NULL)
		panic("Memory map request failed");
	if (addr_request.response == NULL)
		panic("Could not get physical address of kernel");
	printf("Kernel loaded at physical address %p\nHHDM offset is %p\n", addr_request.response->physical_base, hhdm_request.response->offset);
	pmman_init(memory_map_request.response, hhdm_request.response->offset, addr_request.response->physical_base, (uint64_t)&_binary_size);
	tty_puts("Buddy allocator initialization finished.\n");

	size_t pt4;
	asm("mov %0, cr3" : "=r"(pt4));
	printf("Top level page table is at %p\n", pt4);

	kmain();
	panic("kmain returned unexpectedly");
}

void kmain(void) {
	tty_puts("\nPZOS booted successfully!\n");

	// tty_puts("Printing fibonacci sequence:\n");
	// size_t a = 1, b = 1;
	// while (true) {
	// 	size_t c = a + b;
	// 	printf("%u\n", a);
	// 	a = b;
	// 	b = c;

	// 	for (int i = 0; i < 300000000; i++)
	// 		nop();
	// }

	while (true)
		halt();
	panic(NULL);
}
