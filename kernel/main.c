#include <stdbool.h>
#include <stdio.h>

#include <incbin.h>
#include <limine.h>

#include <kernel/intrin.h>
#include <kernel/panic.h>
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

	kmain();

	panic("kmain returned unexpectedly");
}

void kmain(void) {
	tty_puts("PZOS booted successfully!\n");

	while (true)
		halt();

	panic(NULL);
}
