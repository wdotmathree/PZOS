#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <kernel/kbd.h>
#include <kernel/kmalloc.h>
#include <kernel/panic.h>
#include <kernel/tty.h>

char buf[1024];
unsigned buflen = 0;
unsigned bufpos = 0;

char prompt[] = "\x1b[92mPZOS@PZOS\x1b[0m:\x1b[94m/\x1b[0m$ ";
uint8_t promptlen = 13;

static void eval(void) {
	if (buflen == 0) {
		tty_puts(prompt);
		return;
	}

	buf[buflen] = '\0'; // Null-terminate the string
	char *args = strchr(buf, ' ');
	if (args) {
		*args = 0;
		args++;
	}

	if (strcmp(buf, "clear") == 0) {
		tty_clear();
	} else if (strcmp(buf, "help") == 0) {
		tty_puts("Available commands: clear, help, echo\n");
	} else if (strcmp(buf, "echo") == 0) {
		if (args)
			tty_puts(args);
		tty_putchar('\n');
	} else {
		tty_puts(buf);
		tty_puts(": Command not found\n");
	}
}

__attribute__((noreturn)) void kmain(void) {
	tty_putchar('\n');
	tty_puts(prompt);

	while (true) {
		asm("hlt");
		keystroke_t key;
		while ((key = kbd_read()).code != Key_Invalid) {
			if (key.flags & KeyFlag_Released)
				continue;

			if (key.codepoint >= 0x20) { // Printable characters
				tty_putchar(key.codepoint);
				if (bufpos < buflen) {
					memmove(buf + bufpos + 1, buf + bufpos, buflen - bufpos);
					buf[bufpos] = key.codepoint;
					buflen++;
					bufpos++;
					tty_write(buf + bufpos, buflen - bufpos);
					printf("\x1b[%dG", promptlen + bufpos + 1);
				} else {
					buf[bufpos++] = key.codepoint;
					buflen++;
				}
				continue;
			}

			switch (key.code) {
			case Key_Enter:
			case Key_NumEnter:
				tty_putchar('\n');
				eval();
				bufpos = 0;
				buflen = 0;
				tty_puts(prompt);
				break;
			case Key_Backspace:
				if (bufpos == 0)
					break;
				memcpy(buf + bufpos - 1, buf + bufpos, buflen - bufpos);
				bufpos--;
				buflen--;
				tty_puts("\x1b[G\x1b[2K");
				tty_puts(prompt);
				tty_write(buf, buflen);
				printf("\x1b[%dG", promptlen + bufpos + 1);
				break;
			case Key_Left:
				if (bufpos == 0)
					break;
				bufpos--;
				tty_puts("\x1b[1D");
				break;
			case Key_Right:
				if (bufpos == buflen)
					break;
				bufpos++;
				tty_puts("\x1b[1C");
				break;
			}
		}
	}

	while (true)
		hcf();
	panic(NULL);
}
