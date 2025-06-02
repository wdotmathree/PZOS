#include <stdint.h>
#include <stdio.h>
#include <tty.h>

void kernel_early(void) {
	// do stuff
	asm("cli;hlt");
}

void kernel_main(void) {
	terminal_initialize();

	terminal_writestring("Hello, kernel World!\n");

	printf("%d\nabcde\n%s\naaa\n%c", 1234, "Hello, printf World!", 'a');
}
