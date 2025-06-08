#include <kernel/panic.h>

#include <kernel/tty.h>

__attribute__((noreturn)) void _panic(const char *msg, struct isr_frame_t *frame) {
	if (msg != NULL) {
		tty_puts("\nKernel panic: ");
		tty_puts(msg);
		tty_putchar('\n');
	} else {
		tty_puts("Kernel panic: Unknown error.\n");
	}
	tty_puts("System halted.");

	// TODO: Print register state and stack trace

	halt();
}
