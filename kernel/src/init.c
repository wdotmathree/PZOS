#define LIMINE_API_REVISION 3

#include <limine.h>

#include <stdio.h>

#include <kernel/acpi.h>
#include <kernel/efi.h>
#include <kernel/kmalloc.h>
#include <kernel/log.h>
#include <kernel/mman.h>
#include <kernel/panic.h>
#include <kernel/pci.h>
#include <kernel/serial.h>
#include <kernel/time.h>
#include <kernel/tty.h>
#include <kernel/vmem.h>

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
static volatile struct limine_efi_system_table_request efi_system_table_request = {
	.id = LIMINE_EFI_SYSTEM_TABLE_REQUEST,
	.revision = 0,
};

__attribute__((used, section(".limine_requests_end"))) //
static volatile LIMINE_REQUESTS_END_MARKER;

extern void _kernel_end;

__attribute__((noreturn)) void kmain(void);

void test(int a) {
	if (a == 0)
		return;
	return test(a - 1);
}

__attribute__((naked, noreturn)) void kinit(void) {
	// Pop the (bogus) return address to get the stack base
	// Save it in rbp (will be used in mman_init)
	asm volatile("add rsp, 8\n\t"
				 "mov rbp, rsp"
				 : : : "memory");

	// Initialize TSC base (TSC count when the kernel was loaded)
	{
		uint32_t eax = 1, ebx, ecx, edx;
		asm volatile("cpuid"
					 : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx));
		if (edx & (1 << 4)) {
			extern uint64_t tsc_base;
			uint64_t lo, hi;
			asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
			tsc_base = ((uint64_t)hi << 32) | lo;
		}
	}

	// Verify we were loaded correctly
	asm volatile("cli");
	if (!LIMINE_BASE_REVISION_SUPPORTED)
		hcf();
	if (hhdm_request.response == NULL)
		hcf();
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
		hcf();
	}

	// Enable SSE
	asm volatile("mov rax, cr0\n\t"
				 "and rax, -5\n\t" // -5 = 0xfb
				 "or rax, 0x2\n\t"
				 "mov cr0, rax\n\t"
				 "mov rax, cr4\n\t"
				 "or rax, 0x00040600\n\t"
				 "mov cr4, rax"
				 : : : "rax");

	isr_init();

	tty_init(framebuffer_request.response->framebuffers[0]);
	serial_init();

	time_init();

	// Initialize memory management
	if (memory_map_request.response == NULL)
		panic("Memory map request failed");
	extern uint8_t *tty_buf;
	mman_init(memory_map_request.response, &tty_buf, hhdm_request.response->offset, (uintptr_t)&_kernel_end);
	vmem_init();
	kmalloc_init();

	acpi_init(efi_system_table_request.response ? (void *)efi_system_table_request.response->address : NULL);

	pci_init(); // Currently just a stub

	kmain();
}
