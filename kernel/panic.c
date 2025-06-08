#include <panic.h>

#include <tty.h>

__attribute__((noreturn)) void panic(const char *msg) {
	tty_clear();
	if (msg != NULL) {
		tty_puts("Kernel panic: ");
		tty_puts(msg);
		tty_puts("\n");
	} else {
		tty_puts("Kernel panic: Unknown error.\n");
	}
	tty_puts("System halted.");

	// TODO: Print register state and stack trace

	halt();
}
