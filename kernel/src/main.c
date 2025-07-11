#include <stdbool.h>

#include <kernel/kmalloc.h>
#include <kernel/panic.h>
#include <kernel/tty.h>

__attribute__((noreturn)) void kmain(void) {
	tty_puts("\nPZOS booted successfully!\n");

	while (true);

	while (true)
		hcf();
	panic(NULL);
}
