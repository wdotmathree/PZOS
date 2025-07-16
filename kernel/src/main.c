#include <stdbool.h>

#include <kernel/kbd.h>
#include <kernel/kmalloc.h>
#include <kernel/panic.h>
#include <kernel/tty.h>

__attribute__((noreturn)) void kmain(void) {
	tty_puts("\nPZOS booted successfully!\n");

	while (true) {
		asm("hlt");
		keystroke_t key;
		while ((key = kbd_read()).code != Key_Invalid) {
			if (key.flags & KeyFlag_Released)
				continue;

			if (key.codepoint != '\0') {
				tty_putchar(key.codepoint);
				continue;
			}
		}
	}

	while (true)
		hcf();
	panic(NULL);
}
